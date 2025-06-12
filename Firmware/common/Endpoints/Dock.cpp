//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "common/Endpoints/Dock.h"

#include "common/Newton/NSOF.h"

#include "main.h"

#include <stdio.h>
#include <cstring>
//#include <malloc.h>


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


 /* TODO
  - we need a structure that holds a chunk of data
    - the data and the size of the data
    - the current position that we want to send
	- flag if we want to start with a start fram marker
	- flag if we want to end with an end frame marker
	- flag if the data needs to be freed after sending
  - we need a queue of these structures
  - we need a state machine that handles the sending of these structures
	- it needs to handle the start and end frame markers
	- it needs to handle the data size and position
	- it needs to handle the freeing of the data after sending
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

Result Dock::send(Event event) {
	// TODO: for now, simply log all the stuff that we receive.

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
					Log.logf("\r\nERROR: Dock::send: State out of sync!\r\n", cmd_, size);
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


/*
 * This is the command flow to install a package the simple way.
 * Newton | Dock
 *     LR >
 *        < LR
 *     LA >
 *   rtdk > LA			request to dock: len = 4, uint32 protocol version (1 or 4)
 *     LA < dock    	initiate docking: len = 4, uint32 session type
 *   name > LA			Newton name: length, version info, name (UTF-16z)
 *     LA < stim		Set Timeout: timeout in seconds (usually 30 secs)
 *   dres > LA			Result: reply to Frames that don't need any other reply
 *  -- repeat for every file:
 *     LA < lpkg		Load Package: size in byte, binary data (pad to 4)
 *     LA < <data>*n	More binary data until all data is sent
 *   dres > LA			Result: so we know the Newton is still alive
 * -- all files sent:
 *     LA < disc		Disconnect: no data
 *     LD >
 */

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


void Dock::process_command() {
	// TODO: a command can span many MNP frames. The size indicator will tell us hom many bytes we have to read.
	// TODO: `cgfn` gives 0xFFFFFFFF as its size, followed by a NSOF stream. I don't know yet when the command is finished.
	// <16. <10. <02. <02. <04. <0B."newtdockcgfn" <FF. <FF. <FF. <FF. 
	//		NSOF Name: <02. <07. <0D. <47G <65e <74t <55U <73s <65e <72r <43C <6Fo <6En <66f <69i <67g 
	//      NSOF args array: <02. <05. <01. array of 1, followe by the symbol
	//			<07. <0B. <75u <73s <65e <72r <46F <6Fo <6Cl <64d <65e <72r <73s 
	// <10. <03. <D8. <B2. 
	switch (cmd) {
#if 0 // download a fixed package
		case kDRequestToDock: 
			send_cmd_dock(kLoadPackageSession); break;
		case kDNewtonName: // name, and lots of other data
			send_stim(); break;
		case ND_FOURCC('d', 'r', 'e', 's'): // dres, some result for on of the previous messages?
			if (!package_sent) {
				// We have not sent a package yet, so we send the lpkg.
				send_lpkg();
				package_sent = true; // we sent a package
			} else {
				send_disc(); break;
			}
			break;
#else
		case kDRequestToDock: 
			send_cmd_dock(kSettingUpSession); break;
		case kDNewtonName: // name, and lots of other data
			dres_next_ = kDSetTimeout; // If we don't get kDNewtonInfo but dres, just continue
			send_cmd_dinf(); break; // <5F_ <FE. <F6. <6Aj <5B[ <E3. <DA. <62b
		case kDNewtonInfo: // protocol version, encryption key
			// >00. >65e >E0. >BC. >FF. >A7. >74t >1B.
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
			//        >00. >00. >00. >08. >40@ >54T >49I >A1. >CA. >56V >6Cl >4EN
			// 'pass' <00. <00. <00. <08. <366 <5AZ <08. <FF. <A1. <D8. <FC
			// or kDPWWrong and after 3 attempts kDResult / kDBadPassword
			break;

		case kDRequestToBrowse: // rtbr, we assume 'packages
			send_cmd_dres(0); // reply with 0, ok
			break;
			// < kDRequestToBrowse
			// 		ULong 'rtbr'
			// 		ULong length
			// 		NSOF file type: 'import, 'packages(!), 'syncFiles
			// vv-- net needed
			// kDGetInternalStore > optional
			//  	ULong 'gist'
			//		ULong length = 0;
			//		This command requests the Newton to return info about the internal store. The result is described
			//		with the kDInternalStore command.
			// < kDInternalStore
			//		ULong 'isto'
			//		ULong length
			//		NSOF store frame
			//		This command returns information about the internal store. The info is in the form of a frame that
			//		looks like this:
			//		{ name: "Internal",
			//			signature: 27675205,
			//			totalSize: 3608096,
			//			usedSize: 535972,
			//			kind: "Internal"
			//		}
			// ^^-- optional vv-- Windows only
			// kDResult >		// reply with 0
			// < kDGetDevices Windows only
			// kDDevices > Windows only
			// < kDGetFilters Windows only
			// kDFilters > Windows only
			// ^^-- not needed
		case kDGetDefaultPath: // dpth, get the default path
			send_cmd_path();
			break;
			// < kDGetDefaultPath
			//		ULong 'dpth'
			//		ULong length = 0;
			// kDPath >
			//		ULong 'path'
			//		ULong length = 0;
			//		NSOF array of folder frames
			//			[ {name: "Desktop", type: kDesktop},
			//			  {name: "My HD", type: kDesktopDisk, diskType: kHardDrive, whichvol: 0},
			//			  {name: "Business", type: folder} ]
			//		type: desktop = 0, file = 1, folder = 2, disk = 3
			//		diskType: kHardDrive = 0, kFloppyDisk = 1, kCDROM = 2, kNetworkDisk = 3
		case kDGetFilesAndFolders: // gfil, get files and folders
			send_cmd_file();
			break;
			// < kDGetFilesAndFolders
			//		ULong 'gfil'
			//		ULong length = 0;
			// kDFilesAndFolders->
			//		ULong 'file'
			//		ULong length = 0;
			//		NSOF array of file/folder frames
			//			{ name: "Whatever",
			//			  type: kDesktopFolder,
			//			  disktype: 0, // optional if type = disk 
			//			  whichVol: 0, // optional if name is on the desktop
			//			  alias: nil } // optional if it's an alias
			//			[ {name: "Applications", type: kDesktopFolder},
			//			  {name: "important info", type: kDesktopFile},
			//			  {name: "System", type: kDesktopFolder}]
			//		}
			// < kDSetPath
			//		ULong 'spth'
			//		ULong length = 0;
			//		NSOF array of strings: [ "Desktop", {name:"My hard disk", whichVol:0}, "Business" ]
			// kDFilesAndFolders >

			// < kDGetFileInfo
			//		ULong 'gfin'
			//		ULong length
			//		NSOF filename 
			//		The filename is normally a string, but if the selected item is at the Desktop level, a frame
			//		{ name:"Business", whichVol:-1 }
			// kDFileInfo >
			//		ULong 'finf'
			//		ULong length
			//		NSOF info frame
			//		This command is sent in response to a kDGetFileInfo command. It returns a frame that looks like
			//		this:
			//		{ kind: "Microsoft Word document", size: 20480,
			//			created: 3921837, modified: 3434923,
			//			icon: <binary object of icon>,
			//			path: "hd:files:another folder:" }
			//		kind: is a description of the file.
			//		size: is the number of bytes (actual, not the amount used on the disk).
			//		created: is the creation date in Newton date format.
			//		modified: is the modification date of the file.
			//		icon: is an icon to display. This is optional.
			//		path: is the "user understandable" path description

		case kDLoadPackageFile: // lpfl
			handle_LoadPackageFile();
			break;
			// < kDLoadPackageFile
			//		ULong 'lpfl'
			//		ULong length
			//		NSOF filename
			//		If the selected item is at the Desktop level, a frame
			//		{ name:"Business", whichVol:-1 }
			// >02. >08. >0C. >00. >61a >00. >2E. >00. >70p >00. >6Bk >00. >67g >00. >00. >00.
			// kDLoadPackage >
			//		ULong 'lpkg'
			//		ULong length
			//		UChar package data []
			// < kDResult

		case kDSetPath:
		    // >73s >70p >74t >68h >00. >00. >00. >4BK 
			// >02. >05. >04. 
			//   >08. >10. >10. >00. >4EN >00. >65e >00. >77w >00. >74t >00. >43C >00. >4FO >00. >4DM >00. >00. 
			//   >08. >0C. >00. >45E >00. >52R >00. >52R >00. >4FO >00. >52R >00. >00. 
			//   >08. >12. >00. >4DM >00. >79y >00. >46F >00. >6Fo >00. >6Cl >00. >64d >00. >65e >00. >72r >00. >00. 
			//   >08. >12. >00. >4DM >00. >79y >00. >46F >00. >6Fo >00. >6Cl >00. >64d >00. >65e >00. >72r >00. >00. 
			//   >00. >10. >03. >27' >08.
			// [ "Desktop", {name:"My hard disk", whichVol:0}, "Business" ]
			// TODO: Change the path
			// Reply with kDFileAndFolders 
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

#endif
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
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
	});
}

void Dock::send_cmd_stim() {
	if (kLogDock) Log.log("Dock: send_cmd_stim\r\n");
	static std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 's',  't',  'i',  'm', 0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0x0A // 0x00, 0x00, 0x00, 0x5A // TODO: 10 seconds for debugging
	};
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
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
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
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
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
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
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
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
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
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
	// TDCLDockCmdPassword::CreateChallenge( theChallenge );
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
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
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
	});

}



// uint32_t getTotalHeap(void) {
//    extern char __StackLimit, __bss_end__;
   
//    return &__StackLimit  - &__bss_end__;
// }

// uint32_t getFreeHeap(void) {
//    struct mallinfo m = mallinfo();

//    return getTotalHeap() - m.uordblks;
// }


void Dock::send_cmd_path() {
//			[ {name: "Desktop", type: kDesktop},
//			  {name: "My HD", type: kDesktopDisk, diskType: kHardDrive, whichvol: 0},
//			  {name: "Business", type: folder} ]
//		type: desktop = 0, file = 1, folder = 2, disk = 3
//		diskType: kHardDrive = 0, kFloppyDisk = 1, kCDROM = 2, kNetworkDisk = 3
#if 0 // Fixed (cheat) path return
	static const std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 'p',  'a',  't',  'h', //0x00, 0x00, 0x00, 0x00,

		0x00, 0x00, 0x00, 0x65, 
		0x02, //nsof start
		      0x05, 0x04, // Array of four
			              0x06, 0x02, // Frame of 2
						               0x07, 0x04, 0x6E, 
		0x61, 0x6D, 0x65, // Frame.key[0] = 'name
		                  0x07, 0x04, 0x74, 0x79, 0x70, 
		0x65, // Frame.key[1] = 'type
		      0x08, 0x18, 0x00, 0x4D, 0x00, 0x61, 0x00, 
		0x63, 0x00, 0x42, 0x00, 0x6F, 0x00, 0x6F, 0x00, 
	    0x6B, 0x00, 0x20, 0x00, 0x50, 0x00, 0x72, 0x00, 
		0x6F, 0x00, 0x00, // "MacBook Pro" 
		                  0x00, 0x00, // Frame.value[1] = 0 (kDesktop)
                                      0x06, 0x02, // Frame of 2
									              0x09, 
		0x02, // Precendent 2 = 'name
		      0x09, 0x03, // Precendent 3 = 'type
			              0x08, 0x04, 0x00, 0x2F, 0x00, 
		0x00, // Name = "/" , "Users", "matt"
		      0x00, 0x0C, // type = 3 // kDesktopDisk
			              0x06, 0x02, // Frame of 2
						              0x09, 0x02, 0x09, 
		0x03, // 'name, 'type
		      0x08, 0x0C, 0x00, 0x55, 0x00, 0x73, 0x00, 
		0x65, 0x00, 0x72, 0x00, 0x73, 0x00, 0x00, // "Users"
		                                          0x00, 
		0x08, // type 2, // kFolder
		      0x06, 0x02, 0x09, 0x02, 0x09, 0x03, 0x08, 
		0x0A, 0x00, 0x6D, 0x00, 0x61, 0x00, 0x74, 0x00, 
		0x74, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 
	};
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
	});
	if (kLogDock) Log.logf("Dock: send_cmd_path: size = %d\r\n", cmd.size());
	//if (kLogDock) Log.logf("\n\nHeap size = %d\n\n", getFreeHeap());
#else
	static const std::vector<uint8_t> cmd_header = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 'p',  'a',  't',  'h', 0x00, 0x00, 0x00, 0x00,
	};
	
	// NSOF path as an array of folder frames:

	String desktop_name( u"NewtCOM" );
	// String disk_name( u"/" ); // TODO: we can retrieve the name from the SD card
	String disk_name( sdcard_endpoint.get_label() ); // TODO: we can retrieve the name from the SD card
	String folder1_name( u"Users" );
	String folder2_name( u"matt" );

	Frame desktop;
	desktop.add(nd::symName, Ref(desktop_name));
	desktop.add(nd::symType, Ref(kDesktop));

	Frame disk;
	disk.add(nd::symName, Ref(disk_name));
	disk.add(nd::symType, Ref(kDesktopDisk));
	//disk.add(nd::symDiskType, Ref(int32_t(0))); // kHardDrive
	//disk.add(nd::symWhichVol, Ref(int32_t(0))); // whichvol: 0
	
	// Frame folder1;
	// folder1.add(nd::symName, Ref(folder1_name));
	// folder1.add(nd::symType, Ref(kDesktopFolder));

	// Frame folder2;
	// folder2.add(nd::symName, Ref(folder2_name));
	// folder2.add(nd::symType, Ref(kDesktopFolder));

	Array path;
	path.add(Ref(desktop));
	path.add(Ref(disk));
	// path.add(Ref(folder1));
	// path.add(Ref(folder2));
	// *************************************************************************
	// This call breaks things!
	// Adding 'disk' makes the cmd invalid and we receive a NACK from the Newton.
	// Is it the missing "precedent" feature?
	//
	//path.add(Ref(folder));

	Ref(path).logln();

	NSOF nsof;
	nsof.to_nsof(path);
	nsof.log();

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
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = true, // delete the vector after sending it
	});
	if (kLogDock) Log.logf("Dock: send_cmd_path: size = %d (NSOF: %d, aligned: %d)\r\n", cmd->size(), nsof_size, aligned_size);
#endif
}

void Dock::send_cmd_file() { //[{name: "important info", type: kDesktopFile}]
#if 0
	static const std::vector<uint8_t> cmd = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 'f',  'i',  'l',  'e', //0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 33, // 33
		0x02, 0x05, 0x01, // array
		0x06, 0x02, // frame
		0x07, 0x04, 0x6E, 0x61, 0x6D, 0x65, // name
		0x07, 0x04, 0x74, 0x79, 0x70, 0x65, // type
		0x08, 0x0C, 0x00, 'n', 0x00, '.', 0x00, 'p', 0x00, 'k', 0x00, 'g', 0x00, 0x00, 
		0x00, 0x04, // 04 = 1 = File
		0x00, 0x00, 0x00 // align to 4
	};
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
	});
#else
	static const std::vector<uint8_t> cmd_header = {
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		 'f',  'i',  'l',  'e', 0x00, 0x00, 0x00, 0x00,
	};

	Array file_list;
	
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

	Ref(file_list).logln();

	NSOF nsof;
	nsof.to_nsof(file_list);
	//nsof.log();

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
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = true, // delete the vector after sending it
	});
	if (kLogDock) Log.logf("Dock: send_cmd_file: size = %d (NSOF: %d, aligned: %d)\r\n", cmd->size(), nsof_size, aligned_size);
#endif
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
	if (n < 2) {
		if (kLogDock) Log.log("Dock: handle_SetPath: array too short\r\n");
		send_cmd_dres(-48401); // Expected an array with at least 2 elements
		return;
	}
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
	if (path.empty()) path.push_back('/');
	sdcard_endpoint.chdir(path);

	send_cmd_file();
}

void Dock::handle_LoadPackageFile()
{
	// TODO: if current_task_ is not IDLE, send erroro code (other task is running)
	// in_data_ 
	//		ULong 'lpfl'
	//		ULong length
	//		NSOF filename
	//		If the selected item is at the Desktop level, a frame
	//		{ name:"Business", whichVol:-1 }
	// >02. >08. >0C. >00. >61a >00. >2E. >00. >70p >00. >6Bk >00. >67g >00. >00. >00.
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



void Dock::send_lpkg(const std::u16string &filename) {
	// CONT: Continue here: this must really be done in a state machine, because 
	// file sizes can get pretty large and we must not block the task manager.
#if 0
	if (kLogDock) Log.log("Dock: send_lpkg\r\n");
	static std::vector<uint8_t> cmd = {

		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b, 
		0x6c, 0x70, 0x6b, 0x67, 0x00, 0x00, 0x08, 0xa4, // lpkg, 184*12+4
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
	});

	int32_t error_code = 0;
	
	FRESULT f_open (
  FIL* fp,           /* [OUT] Pointer to the file object structure */
  const TCHAR* path, /* [IN] File name */
  BYTE mode          /* [IN] Mode flags */FA_READ|FA_OPEN_EXISTING
);

FSIZE_t f_size (
  FIL* fp   /* [IN] File object */
);

FRESULT f_read (
  FIL* fp,     /* [IN] File object */
  void* buff,  /* [OUT] Buffer to store read data */
  UINT btr,    /* [IN] Number of bytes to read */
  UINT* br     /* [OUT] Number of bytes read */
);

FRESULT f_close (
  FIL* fp     /* [IN] Pointer to the file object */
);

#else
	if (kLogDock) Log.log("Dock: send_lpkg\r\n");
	static std::vector<uint8_t> cmd = {

		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b, 
		0x6c, 0x70, 0x6b, 0x67, 0x00, 0x00, 0x08, 0xa4, // lpkg, 184*12+4

  0x70, 0x61, 0x63, 0x6b, 0x61, 0x67, 0x65, 0x31, 0x78, 0x78, 0x78, 0x78,
  0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x62,
  0x00, 0x62, 0x00, 0x16, 0x00, 0x00, 0x08, 0xa4, 0xaf, 0x7b, 0xb9, 0xd5,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0c,
  0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x98,
  0x00, 0x00, 0x07, 0x98, 0x61, 0x75, 0x74, 0x6f, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x81, 0x00, 0x78, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xa9, 0x00, 0x31, 0x00, 0x39, 0x00, 0x39, 0x00, 0x36, 0x00, 0x20,
  0x00, 0x41, 0x00, 0x70, 0x00, 0x70, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x20,
  0x00, 0x43, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x70, 0x00, 0x75, 0x00, 0x74,
  0x00, 0x65, 0x00, 0x72, 0x00, 0x2c, 0x00, 0x20, 0x00, 0x49, 0x00, 0x6e,
  0x00, 0x63, 0x00, 0x2e, 0x00, 0x20, 0x00, 0x20, 0x00, 0x41, 0x00, 0x6c,
  0x00, 0x6c, 0x00, 0x20, 0x00, 0x72, 0x00, 0x69, 0x00, 0x67, 0x00, 0x68,
  0x00, 0x74, 0x00, 0x73, 0x00, 0x20, 0x00, 0x72, 0x00, 0x65, 0x00, 0x73,
  0x00, 0x65, 0x00, 0x72, 0x00, 0x76, 0x00, 0x65, 0x00, 0x64, 0x00, 0x2e,
  0x00, 0x00, 0x00, 0x53, 0x00, 0x61, 0x00, 0x6c, 0x00, 0x6d, 0x00, 0x6f,
  0x00, 0x6e, 0x00, 0x3a, 0x00, 0x44, 0x00, 0x54, 0x00, 0x53, 0x00, 0x00,
  0x61, 0x75, 0x74, 0x6f, 0x4e, 0x65, 0x77, 0x74, 0x6f, 0x6e, 0xaa, 0x20,
  0x54, 0x6f, 0x6f, 0x6c, 0x4b, 0x69, 0x74, 0x20, 0x50, 0x61, 0x63, 0x6b,
  0x61, 0x67, 0x65, 0x20, 0xa9, 0x20, 0x31, 0x39, 0x39, 0x32, 0x2d, 0x31,
  0x39, 0x39, 0x35, 0x2c, 0x20, 0x41, 0x70, 0x70, 0x6c, 0x65, 0x20, 0x43,
  0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x72, 0x2c, 0x20, 0x49, 0x6e, 0x63,
  0x2e, 0x00, 0xff, 0xff, 0x00, 0x00, 0x10, 0x41, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x1d, 0x00, 0x00, 0x18, 0x43,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x35, 0x00, 0x00, 0x01, 0xb5,
  0x00, 0x00, 0x03, 0x99, 0x00, 0x00, 0x07, 0x59, 0x00, 0x00, 0x1c, 0x41,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x01, 0x51, 0x00, 0x00, 0x01, 0x71, 0x00, 0x00, 0x01, 0x95,
  0x00, 0x00, 0x1e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52,
  0x81, 0x91, 0x61, 0x8c, 0x69, 0x6e, 0x73, 0x74, 0x61, 0x6c, 0x6c, 0x53,
  0x63, 0x72, 0x69, 0x70, 0x74, 0x00, 0xbf, 0xbf, 0x00, 0x00, 0x21, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0x53, 0xe4, 0x69, 0xb3,
  0x64, 0x65, 0x76, 0x49, 0x6e, 0x73, 0x74, 0x61, 0x6c, 0x6c, 0x53, 0x63,
  0x72, 0x69, 0x70, 0x74, 0x00, 0xbf, 0xbf, 0xbf, 0x00, 0x00, 0x20, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0x36, 0x12, 0xb3, 0xf2,
  0x64, 0x65, 0x76, 0x52, 0x65, 0x6d, 0x6f, 0x76, 0x65, 0x53, 0x63, 0x72,
  0x69, 0x70, 0x74, 0x00, 0x00, 0x00, 0x24, 0x43, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xd9, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x02, 0xa9,
  0x00, 0x00, 0x02, 0xc5, 0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x00, 0x04,
  0x00, 0x00, 0x03, 0x35, 0x00, 0x00, 0x28, 0x41, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02, 0x01,
  0x00, 0x00, 0x02, 0x19, 0x00, 0x00, 0x02, 0x39, 0x00, 0x00, 0x02, 0x55,
  0x00, 0x00, 0x02, 0x71, 0x00, 0x00, 0x02, 0x89, 0x00, 0x00, 0x16, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0x25, 0x0b, 0xd4, 0x46,
  0x63, 0x6c, 0x61, 0x73, 0x73, 0x00, 0xbf, 0xbf, 0x00, 0x00, 0x1d, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0x83, 0xa6, 0x3a, 0xcd,
  0x69, 0x6e, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x73,
  0x00, 0xbf, 0xbf, 0xbf, 0x00, 0x00, 0x19, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x55, 0x52, 0xc3, 0xc1, 0x17, 0x60, 0x6c, 0x69, 0x74, 0x65,
  0x72, 0x61, 0x6c, 0x73, 0x00, 0xbf, 0xbf, 0xbf, 0x00, 0x00, 0x19, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0x13, 0xe7, 0x40, 0xdd,
  0x61, 0x72, 0x67, 0x46, 0x72, 0x61, 0x6d, 0x65, 0x00, 0xbf, 0xbf, 0xbf,
  0x00, 0x00, 0x18, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52,
  0x5b, 0x3c, 0x3b, 0xf5, 0x6e, 0x75, 0x6d, 0x41, 0x72, 0x67, 0x73, 0x00,
  0x00, 0x00, 0x1d, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52,
  0x7c, 0xe9, 0xe5, 0xa9, 0x44, 0x65, 0x62, 0x75, 0x67, 0x67, 0x65, 0x72,
  0x49, 0x6e, 0x66, 0x6f, 0x00, 0xbf, 0xbf, 0xbf, 0x00, 0x00, 0x1c, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x19, 0x7b, 0x18, 0x91, 0x19,
  0x81, 0x1a, 0x29, 0xa4, 0x7b, 0x7c, 0x7b, 0x1b, 0x3a, 0x00, 0x7c, 0x02,
  0x00, 0x00, 0x1c, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x39,
  0x00, 0x00, 0x01, 0x95, 0x00, 0x00, 0x02, 0xe1, 0x00, 0x00, 0x03, 0x15,
  0x00, 0x00, 0x01, 0x71, 0x00, 0x00, 0x14, 0x41, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02, 0xf5,
  0x00, 0x00, 0x1d, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52,
  0x63, 0xbf, 0xab, 0xcb, 0x72, 0x65, 0x6d, 0x6f, 0x76, 0x65, 0x53, 0x63,
  0x72, 0x69, 0x70, 0x74, 0x00, 0xbf, 0xbf, 0xbf, 0x00, 0x00, 0x1f, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0xea, 0x16, 0x3c, 0xf7,
  0x45, 0x6e, 0x73, 0x75, 0x72, 0x65, 0x49, 0x6e, 0x74, 0x65, 0x72, 0x6e,
  0x61, 0x6c, 0x00, 0xbf, 0x00, 0x00, 0x18, 0x41, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x03, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x65,
  0x00, 0x00, 0x03, 0x81, 0x00, 0x00, 0x15, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x55, 0x52, 0xfb, 0x0a, 0xc5, 0x8e, 0x64, 0x62, 0x67, 0x31,
  0x00, 0xbf, 0xbf, 0xbf, 0x00, 0x00, 0x1a, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x55, 0x52, 0x8e, 0x0e, 0x79, 0x12, 0x70, 0x61, 0x72, 0x74,
  0x46, 0x72, 0x61, 0x6d, 0x65, 0x00, 0xbf, 0xbf, 0x00, 0x00, 0x17, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0x06, 0x6e, 0x96, 0x95,
  0x72, 0x46, 0x72, 0x61, 0x6d, 0x65, 0x00, 0xbf, 0x00, 0x00, 0x24, 0x43,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xd9, 0x00, 0x00, 0x00, 0x32,
  0x00, 0x00, 0x03, 0xbd, 0x00, 0x00, 0x03, 0xd1, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x07, 0x25, 0x00, 0x00, 0x11, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x19, 0x18, 0x19, 0x1a, 0x32,
  0x02, 0xbf, 0xbf, 0xbf, 0x00, 0x00, 0x18, 0x41, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x02, 0x39, 0x00, 0x00, 0x03, 0xe9, 0x00, 0x00, 0x04, 0x05,
  0x00, 0x00, 0x04, 0xc9, 0x00, 0x00, 0x1b, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x55, 0x52, 0x24, 0xbe, 0x15, 0xb7, 0x53, 0x61, 0x6c, 0x6d,
  0x6f, 0x6e, 0x3a, 0x44, 0x54, 0x53, 0x00, 0xbf, 0x00, 0x00, 0x14, 0x43,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x19, 0x00, 0x00, 0x04, 0x61,
  0x00, 0x00, 0x04, 0xb1, 0x00, 0x00, 0x18, 0x41, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x04, 0x31,
  0x00, 0x00, 0x04, 0x49, 0x00, 0x00, 0x16, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x55, 0x52, 0x8f, 0xa5, 0x88, 0xf2, 0x74, 0x69, 0x74, 0x6c,
  0x65, 0x00, 0xbf, 0xbf, 0x00, 0x00, 0x16, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x55, 0x52, 0xb4, 0xff, 0x1b, 0xc7, 0x63, 0x6f, 0x6c, 0x6f,
  0x72, 0x00, 0xbf, 0xbf, 0x00, 0x00, 0x38, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x04, 0x99, 0x00, 0x53, 0x00, 0x61, 0x00, 0x6c, 0x00, 0x6d,
  0x00, 0x6f, 0x00, 0x6e, 0x00, 0x20, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x20,
  0x00, 0x50, 0x00, 0x75, 0x00, 0x66, 0x00, 0x66, 0x00, 0x20, 0x00, 0x50,
  0x00, 0x61, 0x00, 0x73, 0x00, 0x74, 0x00, 0x72, 0x00, 0x79, 0x00, 0x00,
  0x00, 0x00, 0x17, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52,
  0x18, 0x10, 0xf3, 0x5f, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x00, 0xbf,
  0x00, 0x00, 0x15, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52,
  0x1e, 0x4f, 0x7f, 0x22, 0x70, 0x69, 0x6e, 0x6b, 0x00, 0xbf, 0xbf, 0xbf,
  0x00, 0x00, 0x24, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xd9,
  0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x04, 0xed, 0x00, 0x00, 0x05, 0x21,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x06, 0xe1,
  0x00, 0x00, 0x33, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x19,
  0x18, 0x19, 0x29, 0x1a, 0x29, 0xc5, 0x6f, 0x00, 0x12, 0x18, 0x1b, 0x29,
  0x1c, 0x1d, 0x29, 0x1e, 0x2a, 0x00, 0x18, 0x19, 0x29, 0x7b, 0x1b, 0x29,
  0x7c, 0x98, 0x1f, 0x00, 0x07, 0x1f, 0x00, 0x08, 0x22, 0x24, 0x1f, 0x00,
  0x09, 0x2c, 0x02, 0xbf, 0x00, 0x00, 0x34, 0x41, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x02, 0x39, 0x00, 0x00, 0x05, 0x55, 0x00, 0x00, 0x05, 0x79,
  0x00, 0x00, 0x05, 0x99, 0x00, 0x00, 0x03, 0x15, 0x00, 0x00, 0x05, 0xb1,
  0x00, 0x00, 0x05, 0xcd, 0x00, 0x00, 0x05, 0xe9, 0x00, 0x00, 0x06, 0x09,
  0x00, 0x00, 0x06, 0x25, 0x00, 0x00, 0x06, 0xb9, 0x00, 0x00, 0x24, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0xb6, 0x2a, 0xb9, 0x5b,
  0x43, 0x68, 0x65, 0x7a, 0x44, 0x54, 0x53, 0x52, 0x65, 0x67, 0x69, 0x73,
  0x74, 0x72, 0x79, 0x3a, 0x44, 0x54, 0x53, 0x00, 0x00, 0x00, 0x1d, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0x0c, 0xdd, 0x2d, 0x2a,
  0x47, 0x65, 0x74, 0x47, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x56, 0x61, 0x72,
  0x00, 0xbf, 0xbf, 0xbf, 0x00, 0x00, 0x18, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x55, 0x52, 0xc2, 0x77, 0xc6, 0x0f, 0x49, 0x73, 0x46, 0x72,
  0x61, 0x6d, 0x65, 0x00, 0x00, 0x00, 0x0c, 0x43, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x05, 0xbd, 0x00, 0x00, 0x10, 0x41, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x1b, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0xda, 0x0a, 0xf0, 0x0d,
  0x54, 0x6f, 0x74, 0x61, 0x6c, 0x43, 0x6c, 0x6f, 0x6e, 0x65, 0x00, 0xbf,
  0x00, 0x00, 0x1d, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52,
  0x8b, 0x2e, 0x17, 0xe1, 0x44, 0x65, 0x66, 0x47, 0x6c, 0x6f, 0x62, 0x61,
  0x6c, 0x56, 0x61, 0x72, 0x00, 0xbf, 0xbf, 0xbf, 0x00, 0x00, 0x1c, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0x7e, 0xfe, 0xbe, 0xea,
  0x43, 0x68, 0x65, 0x7a, 0x44, 0x54, 0x53, 0x3a, 0x44, 0x54, 0x53, 0x00,
  0x00, 0x00, 0x24, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xd9,
  0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x06, 0x49, 0x00, 0x00, 0x06, 0x69,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x1e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x19,
  0x18, 0x28, 0x19, 0x91, 0x6f, 0x00, 0x10, 0x18, 0x28, 0x19, 0x91, 0x1a,
  0x40, 0x5f, 0x00, 0x11, 0x22, 0x02, 0xbf, 0xbf, 0x00, 0x00, 0x18, 0x41,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x39, 0x00, 0x00, 0x06, 0x81,
  0x00, 0x00, 0x06, 0x09, 0x00, 0x00, 0x06, 0x99, 0x00, 0x00, 0x18, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0xae, 0xc0, 0x90, 0x04,
  0x47, 0x65, 0x74, 0x52, 0x6f, 0x6f, 0x74, 0x00, 0x00, 0x00, 0x1e, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0xc0, 0x15, 0x2e, 0x3f,
  0x54, 0x68, 0x69, 0x6e, 0x67, 0x73, 0x43, 0x68, 0x61, 0x6e, 0x67, 0x65,
  0x64, 0x00, 0xbf, 0xbf, 0x00, 0x00, 0x26, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x55, 0x52, 0x3e, 0x96, 0x23, 0xc8, 0x41, 0x64, 0x64, 0x50,
  0x72, 0x6f, 0x63, 0x72, 0x61, 0x73, 0x74, 0x69, 0x6e, 0x61, 0x74, 0x65,
  0x64, 0x43, 0x61, 0x6c, 0x6c, 0x00, 0xbf, 0xbf, 0x00, 0x00, 0x18, 0x41,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x4d, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x06, 0xf9, 0x00, 0x00, 0x07, 0x0d, 0x00, 0x00, 0x14, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52, 0xe3, 0xf5, 0x64, 0xf1,
  0x73, 0x79, 0x6d, 0x00, 0x00, 0x00, 0x16, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x55, 0x52, 0x58, 0xa9, 0x99, 0x53, 0x66, 0x72, 0x61, 0x6d,
  0x65, 0x00, 0xbf, 0xbf, 0x00, 0x00, 0x18, 0x41, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x03, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x65,
  0x00, 0x00, 0x07, 0x3d, 0x00, 0x00, 0x1c, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x55, 0x52, 0xe0, 0xc7, 0x45, 0x31, 0x72, 0x65, 0x6d, 0x6f,
  0x76, 0x65, 0x46, 0x72, 0x61, 0x6d, 0x65, 0x00, 0x00, 0x00, 0x24, 0x43,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xd9, 0x00, 0x00, 0x00, 0x32,
  0x00, 0x00, 0x07, 0x7d, 0x00, 0x00, 0x07, 0x8d, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x08, 0x91, 0x00, 0x00, 0x10, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x19, 0x18, 0x19, 0x31, 0x02,
  0x00, 0x00, 0x14, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x39,
  0x00, 0x00, 0x03, 0xe9, 0x00, 0x00, 0x07, 0xa1, 0x00, 0x00, 0x24, 0x43,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xd9, 0x00, 0x00, 0x00, 0x32,
  0x00, 0x00, 0x07, 0xc5, 0x00, 0x00, 0x07, 0xe1, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x08, 0x7d, 0x00, 0x00, 0x1a, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x19, 0x18, 0x19, 0x29, 0x7b,
  0x1a, 0x2a, 0x00, 0x1b, 0x1c, 0x22, 0x24, 0x1d, 0x2c, 0x02, 0xbf, 0xbf,
  0x00, 0x00, 0x24, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x39,
  0x00, 0x00, 0x05, 0x55, 0x00, 0x00, 0x05, 0x79, 0x00, 0x00, 0x08, 0x05,
  0x00, 0x00, 0x06, 0x09, 0x00, 0x00, 0x08, 0x21, 0x00, 0x00, 0x06, 0xb9,
  0x00, 0x00, 0x1b, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x52,
  0x89, 0xe4, 0xc6, 0x90, 0x52, 0x65, 0x6d, 0x6f, 0x76, 0x65, 0x53, 0x6c,
  0x6f, 0x74, 0x00, 0xbf, 0x00, 0x00, 0x24, 0x43, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xd9, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x08, 0x45,
  0x00, 0x00, 0x08, 0x65, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x1e, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x02, 0x19, 0x18, 0x28, 0x19, 0x91, 0x6f, 0x00, 0x10, 0x18,
  0x28, 0x19, 0x91, 0x1a, 0x40, 0x5f, 0x00, 0x11, 0x22, 0x02, 0xbf, 0xbf,
  0x00, 0x00, 0x18, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x39,
  0x00, 0x00, 0x06, 0x81, 0x00, 0x00, 0x06, 0x09, 0x00, 0x00, 0x06, 0x99,
  0x00, 0x00, 0x14, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x4d,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xf9, 0x00, 0x00, 0x14, 0x41,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x4d, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x03, 0x65
	};
	data_queue_.push(Dock::Data {
		.bytes_ = &cmd,
		.pos_ = 0,
		.start_frame_ = true, // we want to start with a start frame marker
		.end_frame_ = true, // we want to end with an end frame marker
		.free_after_send_ = false, // we don't want to free the data after sending
	});
	Log.logf("\r\n---------- %d ---------\r\n", cmd.size());
#endif
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

