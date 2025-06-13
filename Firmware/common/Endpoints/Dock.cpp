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

#include "common/Newton/NSOF.h"

#include "main.h"

#include <stdio.h>
#include <cstring>


typedef unsigned short UniChar;

typedef struct
{
	uint32_t hi;
	uint32_t lo;
} SNewtNonce;


void DESCharToKey(UniChar * inString, SNewtNonce * outKey);
void DESKeySched(SNewtNonce * inKey, SNewtNonce * outKeys);
void DESPermute(const unsigned char * inPermuteTable, uint32_t inHi, uint32_t inLo, SNewtNonce * outKey);

void DESEncode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr);
void DESEncodeNonce(SNewtNonce * initVect, SNewtNonce * outNonce);

void DESDecode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr);
void DESCBCDecode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr, SNewtNonce * a4);
void DESDecodeNonce(SNewtNonce * initVect, SNewtNonce * outNonce);


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
			case Task::PACKAGE_SENT:
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
				if (kLogDock) Log.log("Dock::send: MNP_CONNECTED\r\n");
				connected_ = true;
				break;
			case Event::Subtype::MNP_DISCONNECTED: // data() is index in out_pool
				if (kLogDock) Log.log("Dock::send: MNP_DISCONNECTED\r\n");
				connected_ = false;
				break;
			case Event::Subtype::MNP_FRAME_START: // data() is index in out_pool
				if (kLogDock) Log.log("Dock::send: MNP_FRAME_START\r\n");
				break;
			case Event::Subtype::MNP_FRAME_END: // data() is index in out_pool
				if (kLogDock) Log.log("Dock::send: MNP_FRAME_END\r\n");
				break;
			default:
				if (kLogDock) Log.logf("Dock::send: MNP event %d\r\n", static_cast<int>(event.subtype()));
				break;
		}
	} else if (event.type() == Event::Type::DATA) {
		//if (kLogDock) Log.logf("#%02x ", event.data());
		uint8_t c = event.data();
		switch (in_stream_state_) {
			case  0: 
				if (c == 'n') {
					in_stream_state_++; 
				} else {
					if (kLogDock) Log.logf("\r\nERROR: Dock::send: State out of sync!\r\n", cmd_, size);
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
				if (kLogDock) Log.logf("\r\nDock::send: Receiving '%s' stream with %d bytes.\r\n", cmd_, size);
				in_data_.clear();
				in_index_ = 0;
				// TODO: call some `preprocess_command()`, returning the startegy for reading the rest of the data.
				if (size==0) {
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
					if (kLogDock) Log.logf("Dock::send: Received '%s' command with %d bytes.\r\n", cmd_, size);
					process_command();
					in_stream_state_ = 0; // reset state
				}
				break;
		}
	} else {
		if (kLogDock) Log.logf("Dock::send: Unknown event type %d\r\n", static_cast<int>(event.type()));
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
			if (kLogDock) Log.logf("Dock::process_command: kDResult %d, next command is %08x\r\n", in_data_[3], dres_next_);
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
			if (kLogDock) Log.log("Dock::process_command: kDOperationCanceled\r\n");
			send_cmd_ocaa(); // Confirm operation canceled
			break;
		case kDHello:
			if (kLogDock) Log.log("Dock::process_command: kDHello\r\n");
			// Don't do anything. Receiving the MNP LA Frame seems to be enough for the Newton.
			break;
		default:
			if (kLogDock) Log.logf("Dock::process_command: Unknown command '%s' (%08x)\r\n", cmd_, cmd);
			break;
	}
}


void Dock::send_cmd_dock(uint32_t session_type) {
	if (kLogDock) Log.log("Dock: send_cmd_dock\r\n");
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
	if (kLogDock) Log.log("Dock: send_cmd_stim\r\n");
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
	if (kLogDock) Log.log("Dock: send_cmd_opca\r\n");
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
	if (kLogDock) Log.log("Dock: send_cmd_ocaa\r\n");
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
	if (kLogDock) Log.log("Dock: send_cmd_helo\r\n");
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
	if (kLogDock) Log.log("Dock: send_cmd_dres\r\n");
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
	if (kLogDock) Log.log("Dock: send_cmd_dinf\r\n");
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
	if (kLogDock) Log.log("Dock: send_cmd_wicn\r\n");
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

void Dock::send_cmd_path() {
	// Note: this send always the default initial path: `/Desktop/SD_Card/`
	// Currently this is only calle by `dpth`, but if we find other occasions,
	// we must fix this to return the actual current path.
	static const std::vector<uint8_t> cmd_header = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 'p',  'a',  't',  'h', 0x00, 0x00, 0x00, 0x00,
	};
	
	// NSOF path as an array of folder frames:

	String desktop_name( u"NewtCOM" );
	std::u16string sd_label = sdcard_endpoint.get_label();
	if (sd_label.empty()) {
		sd_label = u"SD Card"; // Default label if not set
	}
	String disk_name( sd_label ); // TODO: we can retrieve the name from the SD card

	Frame desktop;
	desktop.add(nd::symName, Ref(desktop_name));
	desktop.add(nd::symType, Ref(kDesktop));

	Frame disk;
	disk.add(nd::symName, Ref(disk_name));
	disk.add(nd::symType, Ref(kDesktopDisk));

	Array path;
	path.add(Ref(desktop));
	path.add(Ref(disk));

	if (kLogDock) Ref(path).logln();

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
	if (kLogDock) Log.logf("Dock: send_cmd_file: size = %d (NSOF: %d, aligned: %d)\r\n", cmd->size(), nsof_size, aligned_size);
}

void Dock::send_cmd_pass() {
	if (kLogDock) Log.log("Dock: send_cmd_pass\r\n");
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
	if (kLogDock) Log.log("Dock: send_disc\r\n");
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
		if (kLogDock) Log.logf("Dock: handle_SetPath: NSOF error %d\r\n", error_code);
		send_cmd_dres(error_code);
		return;
	}
	std::u16string path;
	Array *arr = path_ref.as_array();
	if (!arr) {
		if (kLogDock) Log.log("Dock: handle_SetPath: reply is not an Array\r\n");
		send_cmd_dres(-48402); // expected an array
		return;
	}
	uint32_t n = arr->size();
	if (n < 1) {
		if (kLogDock) Log.log("Dock: handle_SetPath: array too short\r\n");
		send_cmd_dres(-48401); // Expected an array with at least 2 elements
		return;
	}
	if (n == 1) {
		path_is_desktop_ = true;
	} else {
		path_is_desktop_ = false;
		for (int i = 2; i < n; ++i) {
			String *name = arr->at(i).as_string();
			if (!name) {
				if (kLogDock) Log.logf("Dock: handle_SetPath: item %d is not a String\r\n", i);
				send_cmd_dres(-48401); // Expected a string
				return;
			}
			path.push_back('/');
			path.append(name->str());
		}
	}
	if (path.empty()) path.push_back('/');
	sdcard_endpoint.chdir(path);

	send_cmd_file();
}

void Dock::handle_GetFileInfo()
{
	NSOF nsof_in(in_data_);
	int32_t error_code = 0;
	Ref reply = nsof_in.to_ref(error_code);
	if (error_code != 0) {
		if (kLogDock) Log.logf("Dock: handle_GetFileInfo: NSOF error %d\r\n", error_code);
		send_cmd_dres(error_code);
		return;
	}
	// reply must be a String containing the file name
	String *filename = reply.as_string();
	if (!filename) {
		if (kLogDock) Log.log("Dock: handle_GetFileInfo: reply is not a String\r\n");
		error_code = -48402; // expected a string
		send_cmd_dres(error_code);
		return;
	}
	if (kLogDock) Log.log("Dock: send file info\r\n");
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
		if (kLogDock) Log.logf("Dock: handle_LoadPackageFile: NSOF error %d\r\n", error_code);
		send_cmd_dres(error_code);
		return;
	}
	// reply must be a String containing the file name
	String *filename = reply.as_string();
	if (filename) {
		pkg_filename_ = filename->str();
		current_task_ = Task::SEND_PACKAGE;
	} else {
		if (kLogDock) Log.log("Dock: handle_LoadPackageFile: reply is not a String\r\n");
		error_code = -48402; // expected a string
		// error_code = -10006; // bad parameter
		// error_code = -28004; // kErrorInvalidParameter
		send_cmd_dres(error_code);
	}
}

void Dock::send_package_task() 
{
	if (current_task_ == Task::SEND_PACKAGE) {
		uint32_t err = sdcard_endpoint.openfile(pkg_filename_);
		if (err != FR_OK) {
			if (kLogDock) Log.logf("Dock: send_package_task: openfile error %d\r\n", err);
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
	} else if (current_task_ == Task::CONTINUE_SEND_PACKAGE) {
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
		if (kLogDock) Log.logf("Dock: send_package_task: read_size = %d, pkg_crsr_ = %d, pkg_size_ = %d\r\n", read_size, pkg_crsr_, pkg_size_);
		auto data = new std::vector<uint8_t>(package_size); 
		uint32_t bytes_read = sdcard_endpoint.readfile(data->data(), read_size);
		if (bytes_read != read_size) {
			if (kLogDock) Log.logf("Dock: send_package_task: readfile error %d\r\n", bytes_read);
		}
		data_queue_.push(Dock::Data {
			.bytes_ = data,
			.pos_ = 0,
			.start_frame_ = false, // we want to start with a start frame marker
			.end_frame_ = last_package, // we want to end with an end frame marker
			.free_after_send_ = true, // we don't want to free the data after sending
		});
		if (last_package) {
			current_task_ = Task::PACKAGE_SENT; // we are done sending the package
		}
	} else if (current_task_ == Task::PACKAGE_SENT) {
		// clean up
		sdcard_endpoint.closefile();
		current_task_ = Task::NONE;
	}
}

// Taken from https://github.com/newtonresearch/newton-framework

/* Permuted Choice 1 */
const unsigned char DESPC1Tbl[] =
{
	 7, 15, 23, 31, 39, 47, 55,
	63,  6, 14, 22, 30, 38, 46,
	54, 62,  5, 13, 21, 29, 37,
	45, 53, 61,  4, 12, 20, 28,
	64,
	 1,  9, 17, 25, 33, 41, 49,
	57,  2, 10, 18, 26, 34, 42,
	50, 58,  3, 11, 19, 27, 35,
	43, 51, 59, 36, 44, 52, 60,
	128
};

/* Permuted Choice 2 */
const unsigned char DESPC2Tbl[] =
{
	50, 47, 53, 40, 63, 59, 61, 36,
	49, 58, 43, 54, 41, 45, 52, 60,
	64,
	38, 56, 48, 57, 37, 44, 51, 62,
	19,  8, 29, 23, 13,  5, 30, 20,
	 9, 15, 27, 12, 16, 11, 21,  4,
	26,  7, 14, 18, 10, 24, 31, 28,
	128
};

const unsigned char DESIPInvTbl[] =
{
	24, 56, 16, 48,  8, 40,  0, 32,
	25, 57, 17, 49,  9, 41,  1, 33,
	26, 58, 18, 50, 10, 42,  2, 34,
	27, 59, 19, 51, 11, 43,  3, 35,
	64,
	28, 60, 20, 52, 12, 44,  4, 36,
	29, 61, 21, 53, 13, 45,  5, 37,
	30, 62, 22, 54, 14, 46,  6, 38,
	31, 63, 23, 55, 15, 47,  7, 39,
	128
};

const unsigned char DESPTbl[] = {
	16, 25, 12, 11,
	 3, 20,  4, 15,
	31, 17,  9,  6,
	27, 14,  1, 22,
	30, 24,  8, 18,
	 0,  5, 29, 23,
	13, 19,  2, 26,
	10, 21, 28,  7,
	128
};

const unsigned char DESSBoxes[8][64] = {
	{	13,  1,  2, 15,  8, 13,  4,  8,  6, 10, 15,  3, 11,  7,  1,  4,
		10, 12,  9,  5,  3,  6, 14, 11,  5,  0,  0, 14, 12,  9,  7,  2,
		 7,  2, 11,  1,  4, 14,  1,  7,  9,  4, 12, 10, 14,  8,  2, 13,
		 0, 15,  6, 12, 10,  9, 13,  0, 15,  3,  3,  5,  5,  6,  8, 11 },
	{	 4, 13, 11,  0,  2, 11, 14,  7, 15,  4,  0,  9,  8,  1, 13, 10,
		 3, 14, 12,  3,  9,  5,  7, 12,  5,  2, 10, 15,  6,  8,  1,  6,
		 1,  6,  4, 11, 11, 13, 13,  8, 12,  1,  3,  4,  7, 10, 14,  7,
		10,  9, 15,  5,  6,  0,  8, 15,  0, 14,  5,  2,  9,  3,  2, 12 },
	{	12, 10,  1, 15, 10,  4, 15,  2,  9,  7,  2, 12,  6,  9,  8,  5,
		 0,  6, 13,  1,  3, 13,  4, 14, 14,  0,  7, 11,  5,  3, 11,  8,
		 9,  4, 14,  3, 15,  2,  5, 12,  2,  9,  8,  5, 12, 15,  3, 10,
		 7, 11,  0, 14,  4,  1, 10,  7,  1,  6, 13,  0, 11,  8,  6, 13 },
	{	 2, 14, 12, 11,  4,  2,  1, 12,  7,  4, 10,  7, 11, 13,  6,  1,
		 8,  5,  5,  0,  3, 15, 15, 10, 13,  3,  0,  9, 14,  8,  9,  6,
		 4, 11,  2,  8,  1, 12, 11,  7, 10,  1, 13, 14,  7,  2,  8, 13,
		15,  6,  9, 15, 12,  0,  5,  9,  6, 10,  3,  4,  0,  5, 14,  3 },
	{	 7, 13, 13,  8, 14, 11,  3,  5,  0,  6,  6, 15,  9,  0, 10,  3,
		 1,  4,  2,  7,  8,  2,  5, 12, 11,  1, 12, 10,  4, 14, 15,  9,
		10,  3,  6, 15,  9,  0,  0,  6, 12, 10, 11,  1,  7, 13, 13,  8,
		15,  9,  1,  4,  3,  5, 14, 11,  5, 12,  2,  7,  8,  2,  4, 14 },
	{	10, 13,  0,  7,  9,  0, 14,  9,  6,  3,  3,  4, 15,  6,  5, 10,
		 1,  2, 13,  8, 12,  5,  7, 14, 11, 12,  4, 11,  2, 15,  8,  1,
		13,  1,  6, 10,  4, 13,  9,  0,  8,  6, 15,  9,  3,  8,  0,  7,
		11,  4,  1, 15,  2, 14, 12,  3,  5, 11, 10,  5, 14,  2,  7, 12 },
	{	15,  3,  1, 13,  8,  4, 14,  7,  6, 15, 11,  2,  3,  8,  4, 14,
		 9, 12,  7,  0,  2,  1, 13, 10, 12,  6,  0,  9,  5, 11, 10,  5,
		 0, 13, 14,  8,  7, 10, 11,  1, 10,  3,  4, 15, 13,  4,  1,  2,
		 5, 11,  8,  6, 12,  7,  6, 12,  9,  0,  3,  5,  2, 14, 15,  9 },
	{	14,  0,  4, 15, 13,  7,  1,  4,  2, 14, 15,  2, 11, 13,  8,  1,
		 3, 10, 10,  6,  6, 12, 12, 11,  5,  9,  9,  5,  0,  3,  7,  8,
		 4, 15,  1, 12, 14,  8,  8,  2, 13,  4,  6,  9,  2,  1, 11,  7,
		15,  5, 12, 11,  9,  3,  7, 14,  3, 10, 10,  0,  5,  6,  0, 13 }
};

// --------- Odd Bit Number ---------
// Table to fix the parity of a byte.

const unsigned char kParitizedByte[256] =
{
	0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x07, 0x08, 0x09, 0x0B, 0x0B, 0x0D, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x13, 0x13, 0x15, 0x15, 0x16, 0x17, 0x19, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1F, 0x1F,
	0x20, 0x21, 0x23, 0x23, 0x25, 0x25, 0x26, 0x27, 0x29, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2F, 0x2F,
	0x31, 0x31, 0x32, 0x33, 0x34, 0x35, 0x37, 0x37, 0x38, 0x39, 0x3B, 0x3B, 0x3D, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x43, 0x43, 0x45, 0x45, 0x46, 0x47, 0x49, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4F, 0x4F,
	0x51, 0x51, 0x52, 0x53, 0x54, 0x55, 0x57, 0x57, 0x58, 0x59, 0x5B, 0x5B, 0x5D, 0x5D, 0x5E, 0x5F,
	0x61, 0x61, 0x62, 0x63, 0x64, 0x65, 0x67, 0x67, 0x68, 0x69, 0x6B, 0x6B, 0x6D, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x73, 0x73, 0x75, 0x75, 0x76, 0x77, 0x79, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7F, 0x7F,
	0x80, 0x81, 0x83, 0x83, 0x85, 0x85, 0x86, 0x87, 0x89, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8F, 0x8F,
	0x91, 0x91, 0x92, 0x93, 0x94, 0x95, 0x97, 0x97, 0x98, 0x99, 0x9B, 0x9B, 0x9D, 0x9D, 0x9E, 0x9F,
	0xA1, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA7, 0xA7, 0xA8, 0xA9, 0xAB, 0xAB, 0xAD, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB3, 0xB3, 0xB5, 0xB5, 0xB6, 0xB7, 0xB9, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBF, 0xBF,
	0xC1, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC7, 0xC7, 0xC8, 0xC9, 0xCB, 0xCB, 0xCD, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD3, 0xD3, 0xD5, 0xD5, 0xD6, 0xD7, 0xD9, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDF, 0xDF,
	0xE0, 0xE1, 0xE3, 0xE3, 0xE5, 0xE5, 0xE6, 0xE7, 0xE9, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEF, 0xEF,
	0xF1, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF7, 0xF7, 0xF8, 0xF9, 0xFB, 0xFB, 0xFD, 0xFD, 0xFE, 0xFF
};


/*------------------------------------------------------------------------------
	P r o t o t y p e s
------------------------------------------------------------------------------*/

static void DESip(uint32_t inKeyHi, uint32_t inKeyLo, SNewtNonce * outKey);
static uint32_t DESfrk(uint32_t Khi, uint32_t Klo, uint32_t R);


/*------------------------------------------------------------------------------
	Permute a 64 bit number using the given permute choice table.
	The number is handled in two 32 bit longs.
------------------------------------------------------------------------------*/

void DESPermute(const unsigned char * inPermuteTable, uint32_t inKeyHi, uint32_t inKeyLo, SNewtNonce * outKey)
{
	uint32_t permutedHi, permutedLo;
	uint32_t srcBits;
	unsigned int bitPos;

	do {
		permutedHi = 0;
		// set chosen bit in output
		while ((bitPos = *inPermuteTable++) < 64)
		{
			permutedHi <<= 1;
			if (bitPos < 32)
				srcBits = inKeyLo;
			else
			{
				srcBits = inKeyHi;
				bitPos -= 32;
			}
			if (srcBits & (1 << bitPos))
				permutedHi |= 1;
		}
		// swap high <-> low	
		srcBits = permutedLo;
		permutedLo = permutedHi;
		permutedHi = srcBits;
	} while (bitPos < 128);

	outKey->hi = permutedHi;
	outKey->lo = permutedLo;
}


/*------------------------------------------------------------------------------
	Calculate the Key Schedule.
------------------------------------------------------------------------------*/

void DESKeySched(SNewtNonce * inKey, SNewtNonce * outKeys)
{
	uint32_t rotateSchedule;
	uint32_t permuteKeyHi, permuteKeyLo;
	SNewtNonce permutedKey;

	DESPermute(DESPC1Tbl, inKey->hi << 1, inKey->lo << 1, &permutedKey);
	permuteKeyHi = permutedKey.hi << 4;
	permuteKeyLo = permutedKey.lo << 4;
	for (rotateSchedule = 0xC0810000; rotateSchedule != 0; rotateSchedule <<= 1)
	{
		if (rotateSchedule & 0x80000000)
		{
			permuteKeyHi = (permuteKeyHi << 1) | ((permuteKeyHi >> 27) & 0x0010);
			permuteKeyLo = (permuteKeyLo << 1) | ((permuteKeyLo >> 27) & 0x0010);
		}
		else
		{
			permuteKeyHi = (permuteKeyHi << 2) | ((permuteKeyHi >> 26) & 0x0030);
			permuteKeyLo = (permuteKeyLo << 2) | ((permuteKeyLo >> 26) & 0x0030);
		}
		DESPermute(DESPC2Tbl, permuteKeyHi, permuteKeyLo, outKeys++);
	}
}


/*------------------------------------------------------------------------------
	Generate a key from a unicode string.
	NOTE	This function relies on big-endian byte order!
------------------------------------------------------------------------------*/

void DESCharToKey(UniChar * inString, SNewtNonce * outKey)
{
	SNewtNonce key0 = { 0x57406860, 0x626D7464 };
	SNewtNonce key1;
	SNewtNonce * pKey1;
	SNewtNonce keysArray[16];
	UniChar buf[4];
	unsigned char * src, * dst;
	int i;
	int moreChars;

	for (moreChars = 1; moreChars == 1; )
	{
		// set up keys array
		DESKeySched(&key0, keysArray);

		// initialize buf with 4 UniChars (64 bits) from string
		for (i = 0; i < 4; i++)
		{
			if (moreChars)
			{
				if ((buf[i] = *inString) != 0)
					inString++;
				else
					moreChars = 0;
			}
			else
			{
				buf[i] = 0;
			}
		}

		// set up key1
		key1.hi = (buf[0] << 16) | buf[1];
		key1.lo = (buf[2] << 16) | buf[3];

		// encode it
		pKey1 = &key1;
		DESEncode(keysArray, sizeof(SNewtNonce), &pKey1);

		// use this key to set up next keysArray
		src = (unsigned char *) &key1;
		dst = (unsigned char *) &key0;
		for (i = 0; i < 8; i++)
		{
			// ensure key has odd parity
			*dst++ = kParitizedByte[*src++];
		}
	}

	// return the final key
	*outKey = key0;
}


/*------------------------------------------------------------------------------
	DESip
------------------------------------------------------------------------------*/

static void DESip(uint32_t inKeyHi, uint32_t inKeyLo, SNewtNonce * outKey)
{
	uint32_t d6 = inKeyHi;
	uint32_t d7 = inKeyHi << 16;
	uint32_t a1 = inKeyLo;
	uint32_t a3 = inKeyLo << 16;
	uint32_t resultHi = 0;
	uint32_t resultLo = 0;
	uint32_t temp;
	int i, j;

	for (j = 0; j < 2; j++)
	{
		resultHi = (resultHi >> 1) | (resultHi << 31);
		resultLo = (resultLo >> 1) | (resultLo << 31);

		for (i = 0; i < 8; i++)
		{
			resultLo = (a3 >> 31) | (resultLo << 1);
			a3 <<= 1;
			resultLo = (resultLo >> 31) | (resultLo << 1);

			resultLo = (a1 >> 31) | (resultLo << 1);
			a1 <<= 1;
			resultLo = (resultLo >> 31) | (resultLo << 1);

			resultLo = (d7 >> 31) | (resultLo << 1);
			d7 <<= 1;
			resultLo = (resultLo >> 31) | (resultLo << 1);

			resultLo = (d6 >> 31) | (resultLo << 1);
			d6 <<= 1;
			resultLo = (resultLo >> 31) | (resultLo << 1);

			temp = resultLo;
			resultLo = resultHi;
			resultHi = temp;
		}
	}
	outKey->hi = resultHi;
	outKey->lo = resultLo;
}


/*------------------------------------------------------------------------------
	The cipher function f(R,K)
	Define 8 blocks of 6 bits each from the 32 bit input R.
------------------------------------------------------------------------------*/

static uint32_t DESfrk(uint32_t Khi, uint32_t Klo, uint32_t R)
{
	uint32_t L;
	SNewtNonce permutedR;
	int i;

	L = 0;
	R = (R >> 31) + (R << 1);			// rotate left 1 bit initially
	for (i = 0; i < 8; i++)
	{
		L |= DESSBoxes[i][(R ^ Klo) & 0x3F];
		L = (L << 28) | (L >> 4);		// rotate LR right 4 bits for next iter
		R = (R << 28) | (R >> 4);
		Klo = (Khi << 26) + (Klo >> 6);	// rotate K right 6 bits
		Khi = Khi >> 6;
	}
	// apply permutation function P(L)
	DESPermute(DESPTbl, 0, L, &permutedR);

	// return 32 bit result
	return permutedR.lo;
}


/*------------------------------------------------------------------------------
	Encode.
------------------------------------------------------------------------------*/

void DESEncode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr)
{
	int i;
	SNewtNonce permutedData;
	SNewtNonce * keyPtr = *memPtr;
	SNewtNonce * kPtr;
	uint32_t keyHi, keyLo;

	while ((byteCount -= sizeof(SNewtNonce)) >= 0)
	{
		kPtr = keys;
		keyHi = keyPtr->hi;
		keyLo = keyPtr->lo;
		DESip(keyHi, keyLo, &permutedData);
		keyHi = permutedData.hi;
		keyLo = permutedData.lo;
		for (i = 0; i < 8; i++)
		{
			keyHi ^= DESfrk(kPtr->hi, kPtr->lo, keyLo);	kPtr++;
			keyLo ^= DESfrk(kPtr->hi, kPtr->lo, keyHi);	kPtr++;
		}
		DESPermute(DESIPInvTbl, keyLo, keyHi, &permutedData);
		*keyPtr++ = permutedData;
	}
	*memPtr = keyPtr;
}


void DESCBCEncode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr, SNewtNonce * a4)
{
	int i;
	SNewtNonce permutedData;
	SNewtNonce * keyPtr = *memPtr;
	SNewtNonce * kPtr;
	uint32_t keyHi, keyLo;

	while ((byteCount -= sizeof(SNewtNonce)) >= 0)
	{
		kPtr = keys;
		keyHi = keyPtr->hi;
		keyLo = keyPtr->lo;
		keyHi = (a4->hi ^= keyHi);
		keyLo = (a4->lo ^= keyLo);
		DESip(keyHi, keyLo, &permutedData);
		keyHi = permutedData.hi;
		keyLo = permutedData.lo;
		for (i = 0; i < 8; i++)
		{
			keyHi ^= DESfrk(kPtr->hi, kPtr->lo, keyLo);	kPtr++;
			keyLo ^= DESfrk(kPtr->hi, kPtr->lo, keyHi);	kPtr++;
		}
		DESPermute(DESIPInvTbl, keyLo, keyHi, &permutedData);
		*keyPtr++ = permutedData;
	}
	*memPtr = keyPtr;
}


void DESEncodeNonce(SNewtNonce * initVect, SNewtNonce * outNonce)
{
	SNewtNonce * pNonce;
	SNewtNonce   keysArray[16];

	DESKeySched(initVect, keysArray);
	pNonce = outNonce;
	DESEncode(keysArray, sizeof(SNewtNonce), &pNonce);
}


/*------------------------------------------------------------------------------
	Decode.
------------------------------------------------------------------------------*/

void DESDecode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr)
{
	int i;
	SNewtNonce permutedData;
	SNewtNonce * keyPtr = *memPtr;
	SNewtNonce * kPtr;
	uint32_t keyHi, keyLo;

	while ((byteCount -= sizeof(SNewtNonce)) >= 0)
	{
		kPtr = keys;
		DESip(keyPtr->hi, keyPtr->lo, &permutedData);
		keyHi = permutedData.hi;
		keyLo = permutedData.lo;
		for (i = 0; i < 8; i++)
		{
			keyHi ^= DESfrk(kPtr->hi, kPtr->lo, keyLo);	kPtr++;
			keyLo ^= DESfrk(kPtr->hi, kPtr->lo, keyHi);	kPtr++;
		}
		DESPermute(DESIPInvTbl, keyLo, keyHi, keyPtr++);
	}
	*memPtr = keyPtr;
}


void DESCBCDecode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr, SNewtNonce * a4)
{
	int i;
	SNewtNonce permutedData, huh;
	SNewtNonce * keyPtr = *memPtr;
	SNewtNonce * kPtr;
	uint32_t dataHi, dataLo;

	while ((byteCount -= sizeof(SNewtNonce)) >= 0)
	{
		kPtr = keys;
		huh = **memPtr;
		DESip(huh.hi, huh.lo, &permutedData);
		dataHi = permutedData.hi;
		dataLo = permutedData.lo;
		for (i = 0; i < 8; i++)
		{
			dataHi ^= DESfrk(kPtr->hi, kPtr->lo, dataLo);	kPtr++;
			dataLo ^= DESfrk(kPtr->hi, kPtr->lo, dataHi);	kPtr++;
		}
		DESPermute(DESIPInvTbl, dataLo, dataHi, &permutedData);
		dataHi = permutedData.hi ^ a4->hi;
		dataLo = permutedData.lo ^ a4->lo;
		*a4 = huh;
		keyPtr->hi = dataHi;
		keyPtr->lo = dataLo;
		keyPtr++;
	}
	*memPtr = keyPtr;
}


void DESDecodeNonce(SNewtNonce * initVect, SNewtNonce * outNonce)
{
	SNewtNonce *	pNonce;
	SNewtNonce		keysArray[16];

	DESKeySched(initVect, keysArray);
	pNonce = outNonce;
	DESDecode(keysArray, sizeof(SNewtNonce), &pNonce);
}

