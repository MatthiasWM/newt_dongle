//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

/*
 * Dock protocol for an interactive dock (also see DanteConnectionProtocol.pdf)
 * Newton | Dock
 *   rtdk > 			kDRequestToDock	'rtdk'	// Ask PC to start docking process
 *        < dock    	kDInitiateDocking	'dock'	// data = session type
 *   name > 			kDNewtonName	'name'	// The name of the newton
 *        < dinf        Desktop Info
 *   ninf > 			Newton Info
 *        < wicn        Which Icons
 *   dres >             Result
 *        < stim        kDSetTimeout	'stim' 	// data = # of seconds
 *   pass >             kDPassword	'pass'	// data = encrypted key
 *        < pass		When sent by the Newton, this command returns the key received in the kDDesktopInfo message
						encrypted using the password.
						When sent by the desktop, this command returns the key received in the kDNewtonInfo message
						encrypted using the password.
						See UDES::CreateNewtonKey and UDES::NewtonEncodeBlock in DCL
 * 
 *        < gsto	    kDGetStoreNames	'gsto'	// no data
 *   stor >             NSOF: 02.05.02.06.0A.07.04.name.07.09.signature07.09. (...)
 * 		  < dsnc        kDDesktopControl (desktop is now in control, no reply by Newton), ends in `opdn`
 *        < cgfn        kDCallGlobalFunction 		 'cgfn'
 *   cres >             kDCallResult			'cres'
 *        < stme        kDLastSyncTIme	'stme'	// The time of the last sync
 *   time >             kDCurrentTime	'time'	// The current time on the Newton
 *        < cgfn
 *   cres >
 *        < cgfn
 *   cres >
 *        < opdn        kDOperationDone 'opdn', desktop no longer in control (see `dsnc`)
 *   helo >             kDHello	'helo'	// no data  about every 5 sec
 * 	      < rtbr        kDRequestToBrowse		'rtbr'
 *        < dres
 *   dpth >             kDGetDefaultPath			'dpth' // Get the starting path
 *        < path        kDPath				'path'
 *   gfil >             kDGetFilesAndFolders		'gfil'	// Ask the desktop for files and folders
 *        < file		NOSF
 *   gfin >             kDGetFileInfo			'gfin'
 * 	      < ffin		kDFileInfo				'finf'
 *   lpfl >             kDLoadPackageFile			'lpfl'
 *        < sdef        kDSetStoreToDefault
 *   dres >
 *        < lpkg		kDLoadPackage	'lpkg'	// data = package
 *   dres >
 * 
 *        < disc		kDDisconnect	'disc'	// no data
 *     LD >
 */


#include "common/Endpoints/Dock.h"

#include "common/Newton/DESKey.h"
#include "common/Newton/NSOF.h"

#include "main.h"

#include <stdio.h>
#include <cstring>


using namespace nd; 


/**
 * \class nd::Dock
 * \brief This class implements useful parts of the Newton docking protocol.
 * 
 * The Newton Docking protocol connects NewtoOS machines to the outside world.
 * It's used to sync and backup the device, install software, emulate a 
 * keyboard, and much more.
 * 
 * Technically, the Dock protocol is a full duplex binary data stream. The
 * streams are interpreted as commands followed by a 32 bit size field, then
 * by `size` bytes of data, and finally padded with 0's to a 4 byte boundary.
 *
 * There is not true block synchronisation or error detection on this level.
 * The MNP Filter however indicates the start and end of a block when MNP
 * encoded data was received.
 * 
 * Docking commands always start with `newt` and `dock`, followed by a four byte
 * command code.
 * 
 * \note The docking handshake and protocol is described elsewhere.
 */

void Dock::clear_data_queue_()
{
    while (!data_queue_.empty()) {
		Data &data = data_queue_.front();
		if (data.free_after_send_ && data.bytes_) {
			delete data.bytes_; 
			data.bytes_ = nullptr; // free the data if we are done with it
		}
		data_queue_.pop();
	}
}

void Dock::reset_()
{
	clear_data_queue_();
    in_data_.clear();
	in_data_.reserve(400);
    size = 0;
    aligned_size = 0;
    in_stream_state_ = 0;
    in_index_ = 0;
    dres_next_ = 0;
    connected_ = false;
    hello_timer_ = 0;
    newt_challenge_hi = 0;
    newt_challenge_lo = 0;
    package_sent = false;
    path_is_desktop_ = false;
    current_task_ = Task::NONE;
    pkg_size_ = 0; // size of the package to be loaded
    pkg_size_aligned_ = 0; // size of the package to be loaded, aligned to 4 bytes
    pkg_crsr_ = 0; // current offset in the package
    pkg_filename_.clear(); // filename of the package to be loaded
	cwd_ = u"/";
}


Result Dock::task() {
	if (!data_queue_.empty()) {
		// hello_timer_ = 0; // reset the hello timer if we have data to send
		Data &data = data_queue_.front();
		if (data.start_frame_) {
			// If we have a start frame, we send it first.
			if (out()->send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_START)).rejected()) {
				return Result::REJECTED;
			} else {
				data.start_frame_ = false; // we sent the start frame, no need to send it again
				return Result::OK;
			}
		}
		if (data.pos_ < data.bytes_->size()) {
			// If we have data to send, we send it.
			if (out()->send(Event(Event::Type::DATA, Event::Subtype::NIL, (*data.bytes_)[data.pos_])).rejected()) {
				return Result::REJECTED;
			} else {
				data.pos_++; // we sent the next byte
				return Result::OK;
			}
		}
		if (data.end_frame_) {
			// If we have an end frame, we send it last.
			if (out()->send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_END)).rejected()) {
				return Result::REJECTED;
			} else {
				data.end_frame_ = false; // we sent the end frame, no need to send it again
			}
		}
		data_queue_.pop();
		if (data.free_after_send_ && data.bytes_) {
			delete data.bytes_; 
			data.bytes_ = nullptr; // free the data if we are done with it
		}
		// TODO: delete *data; ????
	} else if (connected_) {
		// If we are conected, but have not sent any data for more than 5 seconds, 
		// we remind the Newton that we are still alive.
		// NOTE: this does not work as expected. The Netwon shows "Waiting for response" when receiving hello.
		// hello_timer_ += scheduler().cycle_time();
		// if (hello_timer_ > 5'000'000) {
		// 	send_cmd_helo();
		// }
	}
	if (data_queue_.size() == 0) { // TOOD: alternate blocks with '<= 2'?
		switch (current_task_) {
			case Task::SEND_PACKAGE:
			case Task::CONTINUE_SEND_PACKAGE:
			case Task::CANCEL_SEND_PACKAGE:
			case Task::PACKAGE_SENT:
			case Task::PACKAGE_CANCELED:
				send_package_task();
				break;
			default:
				break;
		}
	}
	return super::task();
}

Result Dock::send(Event event) 
{
	if (event.type() == Event::Type::MNP) {
		switch (event.subtype()) {
			case Event::Subtype::MNP_CONNECTED: // data() is index in out_pool
				if (kLogDockProgress) Log.log("Dock::send: MNP_CONNECTED\r\n");
				app_status.set(AppStatus::DOCK_CONNECTED);
				connected_ = true;
				break;
			case Event::Subtype::MNP_DISCONNECTED: // data() is index in out_pool
				if (kLogDockProgress) Log.log("Dock::send: MNP_DISCONNECTED\r\n");
				reset_();
				app_status.set(AppStatus::IDLE);
				connected_ = false;
				break;
			case Event::Subtype::MNP_FRAME_START: // data() is index in out_pool
				if (kLogDock) Log.log("Dock::send: MNP_FRAME_START\r\n");
				break;
			case Event::Subtype::MNP_FRAME_END: // data() is index in out_pool
				if (kLogDock) Log.log("Dock::send: MNP_FRAME_END\r\n");
				break;
			case Event::Subtype::MNP_NEGOTIATING:
				if (kLogDock) Log.log("Dock::send: MNP_NEGOTIATING\r\n");
				break;
			default:
				if (kLogDockErrors) Log.logf("Dock::send: MNP event %d\r\n", static_cast<int>(event.subtype()));
				break;
		}
	} else if (event.type() == Event::Type::DATA) {
		// if (kLogDock) Log.logf("#%02x ", event.data());
		uint8_t c = event.data();
		switch (in_stream_state_) {
			case  0: 
				if (c == 'n') {
					in_stream_state_++; 
				} else {
					if (kLogDockErrors) Log.logf("\r\nERROR: Dock::send: State out of sync!\r\n", cmd_, size);
				}
				break; // 'n'
			case  1: if (c == 'e') in_stream_state_++; else in_stream_state_ = 0; break; // 'e'
			case  2: if (c == 'w') in_stream_state_++; else in_stream_state_ = 0; break; // 'w'
			case  3: if (c == 't') in_stream_state_++; else in_stream_state_ = 0; break; // 't'
			case  4: if (c == 'd') in_stream_state_++; else in_stream_state_ = 0; break; // 'd'
			case  5: if (c == 'o') in_stream_state_++; else in_stream_state_ = 0; break; // 'o'
			case  6: if (c == 'c') in_stream_state_++; else in_stream_state_ = 0; break; // 'c'
			case  7: if (c == 'k') in_stream_state_++; else in_stream_state_ = 0; break; // 'k'
			case  8: cmd_[0] = c; in_stream_state_++; break;
			case  9: cmd_[1] = c; in_stream_state_++; break;
			case 10: cmd_[2] = c; in_stream_state_++; break;
			case 11: cmd_[3] = c; in_stream_state_++; break;
			case 12: size = (c<<24); in_stream_state_++; break;
			case 13: size |= (c<<16); in_stream_state_++; break;
			case 14: size |= (c<<8); in_stream_state_++; break;
			case 15: 
				size |= c;
				in_data_.clear();
				in_index_ = 0;
				// TODO: call some `preprocess_command()`, returning the startegy for reading the rest of the data.
				if (size==0) {
					if (kLogDockProgress) Log.logf("\r\nDock::send: Received '%s' with no payload.\r\n", cmd_);
					process_command();
					in_stream_state_ = 0; 
				} else if (size == 0xffffffff) {
					// This is a special case, we don't know how much data to expect.
					// Depending on the command, one or two NSOF objects will follow.
					// Note that the end of the MNP block does not neccessarily mean the end of the NSOF!
					size = 0; // TODO: this is of course wrong, but the best we can currently do
					aligned_size = 0; // no alignment, we will receive data until the next command
					in_stream_state_ = 0; // reset state and wait for the next "newtdock" sequence
				} else {
					aligned_size = (size + 3) & ~3;
					in_stream_state_++;
				}
				break;
			case 16:
				if (in_index_ < aligned_size) {
					if (in_index_ < size) {
						in_data_.push_back(c);
					}
					in_index_++;
				} 
				if (in_index_ == aligned_size) {
					// We have received all the data, process the command.
					if (kLogDockProgress) Log.logf("Dock::send: Received '%s' with %d bytes payload.\r\n", cmd_, size);
					process_command();
					in_stream_state_ = 0; // reset state
				}
				break;
		}
	} else {
		if (kLogDockErrors) Log.logf("Dock::send: Unknown event type %d\r\n", static_cast<int>(event.type()));
	}

	return super::send(event);
}


void Dock::process_command() 
{
	switch (cmd) {
		case kDRequestToDock: 
			send_cmd_dock(kSettingUpSession); break;
		case kDNewtonName: // name, and lots of other data
			dres_next_ = kDSetTimeout;
			send_cmd_dinf(); 
			break;
		case kDNewtonInfo: // protocol version, encryption key
			dres_next_ = kDSetTimeout; // next command to send
			newt_challenge_hi = in_data_[4] << 24 | in_data_[5] << 16 | in_data_[6] << 8 | in_data_[7];
			newt_challenge_lo = in_data_[8] << 24 | in_data_[9] << 16 | in_data_[10] << 8 | in_data_[11];
			send_cmd_wicn(kInstallIcon); break;
		case kDResult:
			if (kLogDockProgress) Log.logf("Dock::process_command: kDResult %d, next command is %08x\r\n", in_data_[3], dres_next_);
			if (kLogDock) Log.log("\r\nDRES: ");
			for (int i=0; i<in_data_.size(); i++) {
				uint8_t c = in_data_[i];
				//if (kLogDock) Log.logf("%02x%c ", in_data_[i], (c>0x20 && c<0x7f) ? c : '.');
			}
			//if (kLogDock) Log.log("\r\n");
			if (dres_next_ == kDSetTimeout) {
				dres_next_ = 0;
				send_cmd_stim();
			}
			break;
		case kDPassword:
			send_cmd_pass(); 
			break;
		case kDRequestToBrowse: // rtbr, we assume 'packages
			send_cmd_dres(0); // reply with 0, ok
			break;
		case kDGetDefaultPath: // dpth, get the default path
			send_cmd_path();
			break;
		case kDGetFilesAndFolders: // gfil, get files and folders
			send_cmd_file();
			break;
		case kDGetFileInfo:
			handle_GetFileInfo();
			break;
		case kDLoadPackageFile: // lpfl
			handle_LoadPackageFile();
			break;
		case kDSetPath:
			handle_SetPath();
			break;
		case kDOperationCanceled:
			if (kLogDockProgress) Log.log("Dock::process_command: kDOperationCanceled\r\n");
			if (current_task_ == Task::CONTINUE_SEND_PACKAGE) {
				if (kLogDockProgress) Log.log("Dock::process_command: Mode = PACKAGE_CANCELED\r\n");
				current_task_ = Task::CANCEL_SEND_PACKAGE; // stop sending the package
			} else {
				send_cmd_ocaa(); // Confirm operation canceled
			}
			break;
		case kDHello:
			if (kLogDockProgress) Log.log("Dock::process_command: kDHello\r\n");
			// Don't do anything. Receiving the MNP LA Frame seems to be enough for the Newton.
			break;
		case kDDisconnect: // `disc`
			if (kLogDockProgress) Log.log("Dock::process_command: kDDisconnect\r\n");
			break;
		default:
			if (kLogDockErrors) Log.logf("Dock::process_command: Unknown command '%s' (%08x)\r\n", cmd_, cmd);
			break;
	}
}


void Dock::send_cmd_dock(uint32_t session_type) {
	if (kLogDockProgress) Log.log("Dock: send_cmd_dock\r\n");
	static std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		0x64, 0x6f, 0x63, 0x6b, 0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0x04
	};
	cmd[19] = session_type;
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true,
		.end_frame_ = true,
		.free_after_send_ = false,
	});
}

void Dock::send_cmd_stim() {
	if (kLogDockProgress) Log.log("Dock: send_cmd_stim\r\n");
	static std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 's',  't',  'i',  'm', 0x00, 0x00, 0x00, 0x04,
#if ND_DEBUG_DOCK
		0x00, 0x00, 0x00, 0x0A // TODO: 10 seconds for debugging
#else
		0x00, 0x00, 0x00, 0x5A 
#endif
	};
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true,
		.end_frame_ = true,
		.free_after_send_ = false,
	});
}

void Dock::send_cmd_opca() {
	if (kLogDockProgress) Log.log("Dock: send_cmd_opca\r\n");
	static std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 'o',  'p',  'c',  'a', 0x00, 0x00, 0x00, 0x00,
	};
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true,
		.end_frame_ = true,
		.free_after_send_ = false,
	});
}

void Dock::send_cmd_ocaa() {
	if (kLogDockProgress) Log.log("Dock: send_cmd_ocaa\r\n");
	static std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 'o',  'c',  'a',  'a', 0x00, 0x00, 0x00, 0x00,
	};
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true,
		.end_frame_ = true,
		.free_after_send_ = false,
	});
}

void Dock::send_cmd_helo() {
	if (kLogDockProgress) Log.log("Dock: send_cmd_helo\r\n");
	static std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 'h',  'e',  'l',  'o', 0x00, 0x00, 0x00, 0x00,
	};
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true,
		.end_frame_ = true,
		.free_after_send_ = false,
	});
}

void Dock::send_cmd_dres(uint32_t error_code) {
	if (kLogDockProgress) Log.log("Dock: send_cmd_dres\r\n");
	static std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 'd',  'r',  'e',  's', 0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0x00
	};
	cmd[16] = (error_code >> 24) & 0xFF;
	cmd[17] = (error_code >> 16) & 0xFF;
	cmd[18] = (error_code >> 8) & 0xFF;
	cmd[19] = error_code & 0xFF;
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true,
		.end_frame_ = true,
		.free_after_send_ = false,
	});
}

void Dock::send_cmd_dinf() {
	if (kLogDockProgress) Log.log("Dock: send_cmd_dinf\r\n");
	static const std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		0x64, 0x69, 0x6e, 0x66, 0x00, 0x00, 0x00, 0x66,

		0x00, 0x00, 0x00, 0x0A, // protocol version (9 or 10)
		0x00, 0x00, 0x00, 0x00, // Desktop Type (0=Mac, 1=Windows)
		0x5F, 0xFE, 0xF6, 0x6A, 0x5B, 0xE3, 0xDA, 0x62, // encrypted key (random challange)
		0x00, 0x00, 0x00, 0x01, // Session Type (kSettingUpSession = 1)
		0x00, 0x00, 0x00, 0x01, // selective sync allowed (only if there was a prevoious sync)
		0x02, 0x05, 0x01, 0x06, 0x04, 0x07, 0x04, 0x6E, 
		0x61, 0x6D, 0x65, 0x07, 0x02, 0x69, 0x64, 0x07, 0x07, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 
		0x07, 0x08, 0x64, 0x6F, 0x65, 0x73, 0x41, 0x75, 0x74, 0x6F, 0x08, 0x24, 0x00, 0x4E, 0x00, 0x65, 
		0x00, 0x77, 0x00, 0x74, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x20, 0x00, 0x43, 0x00, 0x6F, 0x00, 0x6E, 
		0x00, 0x6E, 0x00, 0x65, 0x00, 0x63, 0x00, 0x74, 0x00, 0x69, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 
		0x00, 0x08, 0x00, 0x04, 0x00, 0x1A, 0x00, 0x00, 
		// [ 
		// 	 { name: "Newton Connection"
		//     id: 2
		//     version: 1
		//     doesAuto: true
		// 	 }
		// ]

	};
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true,
		.end_frame_ = true,
		.free_after_send_ = false,
	});
}

void Dock::send_cmd_wicn(uint32_t icon_map) {
	if (kLogDockProgress) Log.log("Dock: send_cmd_wicn\r\n");
	static std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		0x77, 0x69, 0x63, 0x6e, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x04
	};
	cmd[19] = icon_map;
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true,
		.end_frame_ = true,
		.free_after_send_ = false,
	});

}

void Dock::send_cmd_path() 
{
	static const std::vector<uint8_t> cmd_header = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 'p',  'a',  't',  'h', 0x00, 0x00, 0x00, 0x00,
	};

	if (kLogDockProgress) Log.log("Dock: send_cmd_path\r\n");
	
	// NSOF path as an array of folder frames:

	Frame desktop;
	String desktop_name( u"NewtCOM" );
	desktop.add(nd::symName, Ref(desktop_name));
	desktop.add(nd::symType, Ref(kDesktop));

	Array path;
	path.add(Ref(desktop));

	if (!path_is_desktop_) {
		std::u16string sd_label = sdcard_endpoint.get_label();
		if (sd_label.empty()) {
			sd_label = u"SD Card"; // Default label if not set
		}
		String *disk_name = String::New(sd_label);
		Frame *disk = Frame::New();
		disk->add(nd::symName, Ref(disk_name));
		disk->add(nd::symType, Ref(kDesktopDisk));
		path.add(Ref(disk));

		for (auto it = cwd_.begin(); it != cwd_.end(); ) {
			if (*it == '/') { ++it; continue; } // skip slashes
			std::u16string folder_name;
			while (it != cwd_.end() && *it != '/') {
				folder_name += *it;
				++it;
			}
			Frame *folder = Frame::New();
			folder->add(nd::symName, Ref(String::New(folder_name)));
			folder->add(nd::symType, Ref(kDesktopFolder));
			path.add(Ref(folder));
		}
	}

	NSOF nsof;
	nsof.to_nsof(path);
	if (kLogDock) nsof.log();

	std::vector<uint8_t> *cmd = new std::vector<uint8_t>(cmd_header);
	cmd->insert(cmd->end(), nsof.data().begin(), nsof.data().end());

	uint32_t nsof_size = nsof.data().size();
	uint32_t aligned_size = (nsof_size + 3) & 0xfffffffc; // align to 4 bytes
	if (kLogDock) Log.logf("Dock: send_cmd_path: size = %d (NSOF: %d, aligned: %d)\r\n", cmd->size(), nsof_size, aligned_size);
	for (uint32_t i = nsof_size; i < aligned_size; i++) {
		cmd->push_back(0); // fill with 0s to align to 4 bytes
	}
	// The size of the NSOF data is stored at offset 12 in the command data
	(*cmd)[12] = (nsof_size >> 24) & 0xff;
	(*cmd)[13] = (nsof_size >> 16) & 0xff;
	(*cmd)[14] = (nsof_size >> 8) & 0xff;
	(*cmd)[15] = nsof_size & 0xff;

	data_queue_.push(Dock::Data {
		.bytes_ = cmd,
		.pos_ = 0,
		.start_frame_ = true,
		.end_frame_ = true,
		.free_after_send_ = true,
	});
	if (kLogDock) Log.logf("Dock: send_cmd_path: size = %d (NSOF: %d, aligned: %d)\r\n", cmd->size(), nsof_size, aligned_size);
}

void Dock::send_cmd_file() { //[{name: "important info", type: kDesktopFile}]
	if (kLogDockProgress) Log.log("Dock: send_cmd_file\r\n");

	static const std::vector<uint8_t> cmd_header = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 'f',  'i',  'l',  'e', 0x00, 0x00, 0x00, 0x00,
	};

	Array file_list;

	if (path_is_desktop_) {
	
		std::u16string sd_label = sdcard_endpoint.get_label();
		if (sd_label.empty()) {
			sd_label = u"SD Card"; // Default label if not set
		}
		Frame *f = Frame::New();
		f->add(nd::symName, Ref(String::New(sd_label)));
		f->add(nd::symType, Ref(kDesktopDisk));
		file_list.add(Ref(*f));

	} else {

		sdcard_endpoint.opendir();
		std::u16string name;
		for (int i=50; i>0; --i) {
		// for (int i=8; i>0; --i) {
			uint32_t ret = sdcard_endpoint.readdir(name);
			if (ret == FR_IS_DIRECTORY) {
				Frame *f = Frame::New();
				f->add(nd::symName, Ref(String::New(name)));
				f->add(nd::symType, Ref(kDesktopFolder));
				file_list.add(Ref(*f));
			} else if (ret == FR_IS_PACKAGE) {
				Frame *f = Frame::New();
				f->add(nd::symName, Ref(String::New(name)));
				f->add(nd::symType, Ref(kDesktopFile));
				file_list.add(Ref(*f));
			} else {
				break;
			}
		}
		sdcard_endpoint.closedir();

	}

	if (kLogDock) Ref(file_list).logln();

	NSOF nsof;
	nsof.to_nsof(file_list);
	//if (kLogDock) nsof.log();

	std::vector<uint8_t> *cmd = new std::vector<uint8_t>(cmd_header);
	cmd->insert(cmd->end(), nsof.data().begin(), nsof.data().end());

	uint32_t nsof_size = nsof.data().size();
	uint32_t aligned_size = (nsof_size + 3) & 0xfffffffc; // align to 4 bytes
	if (kLogDock) Log.logf("Dock: send_cmd_file: size = %d (NSOF: %d, aligned: %d)\r\n", cmd->size(), nsof_size, aligned_size);
	for (uint32_t i = nsof_size; i < aligned_size; i++) {
		cmd->push_back(0); // fill with 0s to align to 4 bytes
	}
	// The size of the NSOF data is stored at offset 12 in the command data
	(*cmd)[12] = (nsof_size >> 24) & 0xff;
	(*cmd)[13] = (nsof_size >> 16) & 0xff;
	(*cmd)[14] = (nsof_size >> 8) & 0xff;
	(*cmd)[15] = nsof_size & 0xff;

	data_queue_.push(Dock::Data {
		.bytes_ = cmd,
		.pos_ = 0,
		.start_frame_ = true,
		.end_frame_ = true,
		.free_after_send_ = true,
	});
	if (kLogDock) Log.logf("Dock: send_cmd_file: size = %d (NSOF: %d, aligned: %d)\r\n", cmd->size(), nsof_size, aligned_size);
}

void Dock::send_cmd_pass() {
	if (kLogDockProgress) Log.log("Dock: send_cmd_pass\r\n");
	static std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		0x70, 0x61, 0x73, 0x73, 0x00, 0x00, 0x00, 0x08,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};

	SNewtNonce key;
	SNewtNonce newtNonce = { newt_challenge_hi, newt_challenge_lo };
	SNewtNonce response = newtNonce;
	UniChar password[] = { 0x0000 };

	if (kLogDock) Log.logf("Dock: send_cmd_pass: pass = %08lx'%08lx\r\n", newt_challenge_hi, newt_challenge_lo);
	DESCharToKey(password, &key); // hi = 4060593999, lo = 2233144957
	DESEncodeNonce(&key, &response);
	if (kLogDock) Log.logf("Dock: send_cmd_pass: pass = %08lx'%08lx\r\n", response.hi, response.lo);

	cmd[16] = (response.hi >> 24) & 0xff;
	cmd[17] = (response.hi >> 16) & 0xff;
	cmd[18] = (response.hi >> 8) & 0xff;
	cmd[19] = response.hi & 0xff;
	cmd[20] = (response.lo >> 24) & 0xff;
	cmd[21] = (response.lo >> 16) & 0xff;
	cmd[22] = (response.lo >> 8) & 0xff;
	cmd[23] = response.lo & 0xff;

	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
	});
}

void Dock::send_disc() {
	if (kLogDockProgress) Log.log("Dock: send_disc\r\n");
	static std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		0x64, 0x69, 0x73, 0x63, 0x00, 0x00, 0x00, 0x00
	};
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
	});
}

void Dock::handle_SetPath() 
{
	NSOF nsof(in_data_);
	int32_t error_code = 0;
	Ref path_ref = nsof.to_ref(error_code);
	if (error_code != 0) {
		if (kLogDockErrors) Log.logf("Dock: handle_SetPath: NSOF error %d\r\n", error_code);
		send_cmd_dres(error_code);
		return;
	}
	cwd_.clear();
	Array *arr = path_ref.as_array();
	if (!arr) {
		if (kLogDockErrors) Log.log("Dock: handle_SetPath: reply is not an Array\r\n");
		send_cmd_dres(-48402); // expected an array
		return;
	}
	uint32_t n = arr->size();
	if (n < 1) {
		if (kLogDockErrors) Log.log("Dock: handle_SetPath: array too short\r\n");
		send_cmd_dres(-48401); // Expected an array with at least 2 elements
		return;
	}
	if (n == 1) {
		path_is_desktop_ = true;
		cwd_ = u"/";
	} else {
		path_is_desktop_ = false;
		for (int i = 2; i < n; ++i) {
			String *name = arr->at(i).as_string();
			if (!name) {
				if (kLogDockErrors) Log.logf("Dock: handle_SetPath: item %d is not a String\r\n", i);
				send_cmd_dres(-48401); // Expected a string
				return;
			}
			cwd_.push_back('/');
			cwd_.append(name->str());
		}
	}
	if (cwd_.empty()) cwd_.push_back('/');
	sdcard_endpoint.chdir(cwd_);

	send_cmd_file();
}

void Dock::handle_GetFileInfo()
{
	NSOF nsof_in(in_data_);
	int32_t error_code = 0;
	Ref reply = nsof_in.to_ref(error_code);
	if (error_code != 0) {
		if (kLogDockErrors) Log.logf("Dock: handle_GetFileInfo: NSOF error %d\r\n", error_code);
		send_cmd_dres(error_code);
		return;
	}
	// reply must be a String containing the file name
	String *filename = reply.as_string();
	if (!filename) {
		if (kLogDockErrors) Log.log("Dock: handle_GetFileInfo: reply is not a String\r\n");
		error_code = -48402; // expected a string
		send_cmd_dres(error_code);
		return;
	}
	if (kLogDockProgress) Log.log("Dock: send file info 'finf'\r\n");
	static std::vector<uint8_t> cmd_header = {
		'n', 'e', 'w', 't', 'd', 'o', 'c', 'k',
		'f', 'i', 'n', 'f', 0x00, 0x00, 0x00, 0x85,

		// <02. 
		// <06. <06. 
		// <07. <04. <6Bk <69i <6En <64d  // kind
		// <07. <04. <73s <69i <7Az <65e  // size
		// <07. <07. <63c <72r <65e <61a <74t <65e <64d // created
		// <07. <08. <6Dm <6Fo <64d <69i <66f <69i <65e <64d // modified
		// <07. <04. <70p <61a <74t <68h // path
		// <07. <04. <69i <63c <6Fo <6En // icon
		// <08. <2E. // Installer flat package
		//  <00. <49I <00. <6En <00. <73s <00. <74t 
		//  <00. <61a <00. <6Cl <00. <6Cl <00. <65e 
		//  <00. <72r <00. <20  <00. <66f <00. <6Cl 
		//  <00. <61a <00. <74t <00. <20  <00. <70p 
		//  <00. <61a <00. <63c <00. <6Bk <00. <61a 
		//  <00. <67g <00. <65e <00. <00. 
		// <00. <FF. <00. <00. <26& <E0. 
		// <00. <FF. <0E. <92. <27' <44D 
		// <00. <FF. <0F. <14. <C0. <A0. 
		// <08. <12. // test.pkg
		//  <00. <74t <00. <65e <00. <73s <00. <74t
		//  <00. <2E. <00. <70p <00. <6Bk <00. <67g 
		//  <00. <00. 
		// <0A. // NIL
		// <00. <00. <00. // padding
		// <10. <03. <92. <47G // end, checksum

	};

	sdcard_endpoint.openfile(filename->str());
	uint32_t file_size = sdcard_endpoint.filesize();
	sdcard_endpoint.closefile();

	Frame info;
	info.add(nd::symKind, Ref(String::New( { 'P', 'a', 'c', 'k', 'a', 'g', 'e' } ))); // kind
	info.add(nd::symSize, Ref((int32_t)file_size)); // size
	info.add(nd::symCreated, Ref(0)); // created
	info.add(nd::symModified, Ref(0)); // modified
	info.add(nd::symPath, reply); // path
	info.add(nd::symIcon, Ref(false)); // icon

	NSOF nsof;
	nsof.to_nsof(info);
	if (kLogDock) nsof.log();

	std::vector<uint8_t> *cmd = new std::vector<uint8_t>(cmd_header);
	cmd->insert(cmd->end(), nsof.data().begin(), nsof.data().end());

	uint32_t nsof_size = nsof.data().size();
	uint32_t aligned_size = (nsof_size + 3) & 0xfffffffc; // align to 4 bytes
	if (kLogDock) Log.logf("Dock: send_cmd_path: size = %d (NSOF: %d, aligned: %d)\r\n", cmd->size(), nsof_size, aligned_size);
	for (uint32_t i = nsof_size; i < aligned_size; i++) {
		cmd->push_back(0); // fill with 0s to align to 4 bytes
	}
	// The size of the NSOF data is stored at offset 12 in the command data
	(*cmd)[12] = (nsof_size >> 24) & 0xff;
	(*cmd)[13] = (nsof_size >> 16) & 0xff;
	(*cmd)[14] = (nsof_size >> 8) & 0xff;
	(*cmd)[15] = nsof_size & 0xff;

	data_queue_.push(Dock::Data {
		.bytes_ = cmd,
		.pos_ = 0,
		.start_frame_ = true,
		.end_frame_ = true,
		.free_after_send_ = true,
	});
}

void Dock::handle_LoadPackageFile()
{
	// TODO: if current_task_ is not IDLE, send error code (other task is running)
	// in_data_ 
	//		ULong 'lpfl'
	//		ULong length
	//		NSOF filename
	NSOF nsof(in_data_);
	int32_t error_code = 0;
	Ref reply = nsof.to_ref(error_code);
	if (error_code != 0) {
		if (kLogDockErrors) Log.logf("Dock: handle_LoadPackageFile: NSOF error %d\r\n", error_code);
		send_cmd_dres(error_code);
		return;
	}
	// reply must be a String containing the file name
	String *filename = reply.as_string();
	if (filename) {
		if (kLogDockProgress) Log.log("Dock: start to send package file\r\n");
		pkg_filename_ = filename->str();
		current_task_ = Task::SEND_PACKAGE;
	} else {
		if (kLogDockErrors) Log.log("Dock: handle_LoadPackageFile: reply is not a String\r\n");
		error_code = -48402; // expected a string
		// error_code = -10006; // bad parameter
		// error_code = -28004; // kErrorInvalidParameter
		send_cmd_dres(error_code);
	}
}

void Dock::send_package_task() 
{
	if (current_task_ == Task::SEND_PACKAGE) {
		if (kLogDockProgress) Log.log("Dock: start SEND_PACKAGE\r\n");
		uint32_t err = sdcard_endpoint.openfile(pkg_filename_);
		if (err != FR_OK) {
			if (kLogDockErrors) Log.logf("Dock: send_package_task: openfile error %d\r\n", err);
			send_cmd_dres(-48403); // file not found
			current_task_ = Task::NONE;
			return;
		}
		pkg_size_ = sdcard_endpoint.filesize();
		pkg_size_aligned_ = (pkg_size_ + 3) & 0xfffffffc; // align to 4 bytes
		pkg_crsr_ = 0;

		static std::vector<uint8_t> cmd = {
			0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b, 
			 'l',  'p',  'k',  'g', 0x00, 0x00, 0x08, 0x00, // lpkg
		};
		cmd[12] = (pkg_size_ >> 24) & 0xff; // size of the package
		cmd[13] = (pkg_size_ >> 16) & 0xff;
		cmd[14] = (pkg_size_ >> 8) & 0xff;
		cmd[15] = pkg_size_ & 0xff;
		data_queue_.push(Dock::Data {
			.bytes_ = &cmd,
			.pos_ = 0,
			.start_frame_ = true, // we want to start with a start frame marker
			.end_frame_ = false, // we want to end with an end frame marker
			.free_after_send_ = false, // we don't want to free the data after sending
		});
		current_task_ = Task::CONTINUE_SEND_PACKAGE;
	} else if (current_task_ == Task::CONTINUE_SEND_PACKAGE || current_task_ == Task::CANCEL_SEND_PACKAGE) {
		if (kLogDockProgress) Log.log("Dock: continue SEND_PACKAGE\r\n");
		uint32_t read_size = pkg_size_ - pkg_crsr_;
		if (read_size > 512) {
			read_size = 512; // read at most 512 bytes
		}
		uint32_t package_size = read_size;
		pkg_crsr_ += read_size;
		bool last_package = (pkg_crsr_ >= pkg_size_);
		if (last_package) {
			package_size += (pkg_size_aligned_ - pkg_size_); // add padding to align to 4 bytes
		}
		if (current_task_ == Task::CANCEL_SEND_PACKAGE) last_package = true; // we want to cancel the package
		if (kLogDock) Log.logf("Dock: send_package_task: read_size = %d, pkg_crsr_ = %d, pkg_size_ = %d\r\n", read_size, pkg_crsr_, pkg_size_);
		auto data = new std::vector<uint8_t>(package_size); 
		uint32_t bytes_read = sdcard_endpoint.readfile(data->data(), read_size);
		if (bytes_read != read_size) {
			if (kLogDockErrors) Log.logf("Dock: send_package_task: readfile error %d\r\n", bytes_read);
		}
		data_queue_.push(Dock::Data {
			.bytes_ = data,
			.pos_ = 0,
			.start_frame_ = false, // we want to start with a start frame marker
			.end_frame_ = last_package, // we want to end with an end frame marker
			.free_after_send_ = true, // we don't want to free the data after sending
		});
		if (last_package) {
			if (current_task_ == Task::CANCEL_SEND_PACKAGE) {
				current_task_ = Task::PACKAGE_CANCELED;
			} else {
				current_task_ = Task::PACKAGE_SENT; // we are done sending the package
			}
		}
	} else if (current_task_ == Task::PACKAGE_SENT || current_task_ == Task::PACKAGE_CANCELED) {
		// clean up
		if (kLogDockProgress) Log.log("Dock: PACKAGE_SENT\r\n");
		sdcard_endpoint.closefile();
		if (current_task_ == Task::PACKAGE_CANCELED) {
			send_cmd_ocaa();
		}
		current_task_ = Task::NONE;
	}
}

/* Tapping [X] while the package is sent from the desktop to the Newton:

D[128,2480]>16. >10. >02. >03. >05. >1F. >01. >10. >03. >344 >9B. 
<16. <10. <02. 
<02. <04. <20  <00. <00. <00. <00. <00. <00. <01. <00. <00. <00. <00. <00. <00.
<00. <0C. <00. <04. <01. <68h <00. <00. <00. <56V <00. <40@ <00. <05. <00. <16. 
<00. <7F. <00. <A5. <00. <AC. <00. <B1. <00. <B8. <00. <BB. <00. <CF. <00. <D6. 
<00. <DC. <00. <EF. <00. <FC. <00. <FF. <01. <311 <01. <53S <01. <78x <01. <92. 
<02. <C7. <02. <DD. <03. <C0. <20  <14. <20  <1A. <20  <1E. <20  <22" <20  <26& 
<20  <300 <20  <3A: <20  <44D <21! <22" <21! <26& <22" <02. <22" <06. <22" <0F. 
<22" <11. <22" <1A. <22" 
	>16. 
<1E. 
	>10. 
<22" 
	>02. 
<2B+ 
	>02. 
<22" 
	>04. 
<48H 
	>0B. 
<22" <60` <22" <65e <25% <CA. 
	>6En 
<F7. 
	>65e 
<FF. 
	>77w 
<FB. 
	>74t 
<02. 
	>64d 
<FF. 
	>6Fo 
<FF. 
	>63c 
<00. 
	>6Bk 
<00. 
	>6Fo 
<00. 
	>70p 
<01. 
	>63c 
<00. 
	>61a 
<A0. 
	>00. 
<00. 
	>00. 
<A7. 
	>00. 
<00. 
	>00. 
<AE. <00. <B4. <00. <BA. <00. <BF. <00. <D1. <00. <D8. <00. <DF. <00. <F1. <00. 
<FF. <01. <311 <01. <52R <01. <78x <01. <92. <02. <C6. <02. <D8. <03. <C0. <20  
<13. <20  <18. <20  <1C. <20  <20  <20  <26& <20  <300 <20  <399 <20  <44D <21! 
<22" <21! <26& <22" <02. <22" <06. <22" <0F. <22" <11. <22" <1A. <22" <1E. <22" 
<2B+ 
	>10. 
<22" 
	>03. 
<48H 
	>73s 
<22" 
	>AE. 
ERROR: Dock::send: State out of sync!

ERROR: Dock::send: State out of sync!
<60` 
ERROR: Dock::send: State out of sync!

ERROR: Dock::send: State out of sync!

ERROR: Dock::send: State out of sync!

*/