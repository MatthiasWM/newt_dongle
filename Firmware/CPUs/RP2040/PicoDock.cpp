//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoDock.h"

#include "main.h"

#include <stdio.h>
#include <cstring>

using namespace nd; 

static uint16_t kCRC16Table[256] = {
    0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
    0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
    0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
    0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
    0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
    0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
    0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
    0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
    0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
    0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
    0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
    0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
    0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
    0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
    0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
    0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
    0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
    0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
    0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
    0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
    0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
    0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
    0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
    0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
    0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
    0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
    0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
    0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
    0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
    0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
    0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
    0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};

static void update_crc16(unsigned short *pCrc16, const char data[], size_t length) {
	for (size_t i = 0; i < length; i++) {
		uint16_t in_crc = *pCrc16;
		*pCrc16 = (uint16_t)(((in_crc >> 8) & 0xFF) ^ kCRC16Table[ (in_crc & 0xFF) ^ data[i] ]);
	}
}


PicoDock::Frame::Frame() 
{
	header.reserve(256);
	data.reserve(1024);
}

PicoDock::Frame::Frame(const std::vector<uint8_t> &preset_header, const std::vector<uint8_t> &preset_data) 
{
}

void PicoDock::Frame::clear() {
	header.clear();
	data.clear();
	header_size = 0;
	crc = 0;
	type = 0;
	escaping_dle = false;
	crc_valid = false;
}

uint16_t PicoDock::Frame::calculate_crc() {
	static uint8_t ETX = 0x03;
	uint16_t my_crc = 0;
	update_crc16(&my_crc, (const char*)&header_size, 1);
	update_crc16(&my_crc, (const char*)header.data(), header.size());
	update_crc16(&my_crc, (const char*)data.data(), data.size());
	update_crc16(&my_crc, (const char*)&ETX, 1);
	return my_crc;
}

void PicoDock::Frame::prepare_to_send() {
	header_size = header.size();
	type = header[0];
	crc = calculate_crc();
	crc_valid = true;
}

void PicoDock::Frame::print() {
	printf("--- Frame ---\r\n");
	switch (type) {
		case kMNP_Frame_LR: printf("Link Request\r\n"); break;
		case kMNP_Frame_LD: printf("Link Disconnect\r\n"); break;
		case kMNP_Frame_LT: printf("Link Transfer\r\n"); break;
		case kMNP_Frame_LA: printf("Link Acknowledge\r\n"); break;
		default: printf("Unknown Frame Type %d\r\n", type); break;
	}
	printf("Header: ");
	for (auto c : header) {
		printf("%02x ", c);
	}
	printf("\r\n");
	printf("Data: ");
	for (auto c : data) {
		printf("%02x ", c);
	}
	printf("\r\n");
	if (crc_valid) {
		printf("CRC: %04x\r\n", crc);
	} else {
		printf("CRC: invalid\r\n");
	}
	printf("--- Frame End ---\r\n");
}



PicoDock::PicoDock(Scheduler &scheduler) 
:   Endpoint(scheduler)
{
	// TODO: in and out frames are managed in a pool
    in_frame = new Frame();
}

PicoDock::~PicoDock() {
    delete in_frame;
}

Result PicoDock::init() {
	return Endpoint::init();
}

Result PicoDock::task() {
	// We need a FIFO of MNP frames to handle the outgoing data stream.
	// `send()` will receive LA acknowledgements and may request that we send
	// a frame again if it did not get acknowledged or timed out.

	// TODO: handle various timeouts and trigger retries or disconnects.
	if (out_frame) {
		// Continue to send the current frame to "out".
		state_machine_reply(out_frame);
	} else if (job_list.size() > 0) {
		Event event = job_list.front();
		job_list.pop();
		if (event.type() != Event::Type::MNP) {
			Log.log("MNP Error: job list not MNP\r\n");
			return Result::OK__NOT_HANDLED;
		}
		switch (event.subtype()) {
			case Event::Subtype::MNP_SEND_LA: // data() is in sequence number
				break;
			case Event::Subtype::MNP_SEND_LD: // data() is disconnect reason
				break;
			case Event::Subtype::MNP_SEND_LR: // data() is original LR index on in_pool
				break;
			case Event::Subtype::MNP_SEND_LT: // data() is index in out_pool
				break;
		}
	} else {
		Log.log("MNP Error: unknown job\r\n");
		return Result::OK__NOT_HANDLED;
	}
	return Result::OK;
}

void PicoDock::state_machine_reply(Frame *frame){
	Pipe *o = out();
	// TODO : check o and mark frame as sent if o is null.
	switch (out_state_) {
		case OutState::SEND_SYN:
			if (o->send(0x16).ok()) out_state_ = OutState::SEND_DLE;
			break;
		case OutState::SEND_DLE:
			if (o->send(0x10).ok()) out_state_ = OutState::SEND_STX;
			break;
		case OutState::SEND_STX:
			if (o->send(0x02).ok()) out_state_ = OutState::SEND_HDR_SIZE;
			break;
		case OutState::SEND_HDR_SIZE:
			if (o->send(frame->header_size).ok()) {
				data_index_ = 0;
				out_state_ = OutState::SEND_HDR_DATA;
			}
			break;
		case OutState::SEND_HDR_DATA:
			// FIXME: header size can be 0x10 and must be escaped!
			if (data_index_ < frame->header_size) {
				if (o->send(frame->header[data_index_]).ok()) {
					data_index_++;
				}
			} 
			if (data_index_ == frame->header_size) {
				data_index_ = 0;
				out_state_ = OutState::SEND_DATA;
			}
			break;
		case OutState::SEND_DATA:
			if (data_index_ < frame->data.size()) {
				if (o->send(frame->data[data_index_]).ok()) {
					data_index_++;
				}
			}
			if (data_index_ == frame->data.size()) {
				data_index_ = 0;
				out_state_ = OutState::SEND_DLE2;
			}
			break;
		case OutState::SEND_DLE2:
			if (o->send(0x10).ok()) out_state_ = OutState::SEND_ETX;
			break;
		case OutState::SEND_ETX:
			if (o->send(0x03).ok()) out_state_ = OutState::SEND_CRC_lo;
			break;
		case OutState::SEND_CRC_lo:
			if (o->send(frame->crc & 0x00FF).ok()) out_state_ = OutState::SEND_CRC_hi;
			break;
		case OutState::SEND_CRC_hi:
			if (o->send(frame->crc >> 8).ok()) {
				out_state_ = OutState::SEND_SYN;
				out_frame = nullptr; // TODO: mark frame as sent
			}
			break;
	}
}

/**
 * \brief Receive an MNP stream through the in pipe.
 * Handle MNP header, footer, and checksum. 
 * Handle MNP link requests (LR), reply with (LR), handle incomming link acknowledge (LA), and link disconnect (LD).
 * Handle incoming data (LT) and reply with (LR) or (LA), handle 'newt' 'dock' '....'
 * Handle incomming (LR) and (LA) for (LT) frames that we sent.
 * Handle Link Disconnect (LD).
 */
Result PicoDock::send(Event event) {
	// All blocks start with 0x16, 0x10, 0x02 and end with 0x10, 0x03, CRC, CRC,
	// So we extract the header and the data into seperate buffers while we 
	// receive the data stream.
	// If the CRC is correct in the end, we can manage cotrol frames right away,
	// and forward data blocks to the client.
	
	// TODO: Add a timeouts, send the correct LA frames.

	// TODO: let's have a second state machine run parallel that watches for 
	// the 0x16 0x10 0x02 sequnce, which can't be anywhere but at the start of a 
	// frame. If so, abort the current frame and start over.

	uint16_t my_crc;

	if (event.type() != Event::Type::DATA) {
		return Endpoint::send(event);
	}

	uint8_t c = event.data();
	switch (in_state_) {
		// -- wait for 0x16 0x10 0x02
		case InState::WAIT_FOR_SYN:
			if (c == 0x16) in_state_ = InState::WAIT_FOR_DLE;
			break;
		case InState::WAIT_FOR_DLE:
			if (c == 0x10) in_state_ = InState::WAIT_FOR_STX;
			else in_state_ = InState::ABORT;
			break;
		case InState::WAIT_FOR_STX:
			if (c == 0x02) {
				in_frame->clear();
				in_state_ = InState::WAIT_FOR_HDR_SIZE;
			} else {
				in_state_ = InState::ABORT;
			}
			break;

		// -- read the header
        case InState::WAIT_FOR_HDR_SIZE:  // 0..254
			if ((c == 0xff) || (c == 0)) {
				printf("MNP ERROR: unsupported header size %d.\r\n", c);
				in_state_ = InState::ABORT;
				break;
			}
			in_frame->header_size = c;
			in_state_ = InState::WAIT_FOR_HDR_TYPE;
			break;
        case InState::WAIT_FOR_HDR_TYPE:
			in_frame->header.push_back(c);
			if (c == kMNP_Frame_LR) {
				Log.log("kMNP_Frame_LR\r\n");
				in_frame->type = kMNP_Frame_LR;
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else if (c == kMNP_Frame_LD) {
				Log.log("kMNP_Frame_LD\r\n");
				in_frame->type = kMNP_Frame_LD;
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else if (c == kMNP_Frame_LT) {
				Log.log("kMNP_Frame_LT\r\n");
				in_frame->type = kMNP_Frame_LT;
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else if (c == kMNP_Frame_LA) {
				Log.log("kMNP_Frame_LA\r\n");
				in_frame->type = kMNP_Frame_LA;
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else {
				Log.log("kMNP_Frame_UNKNOWN");
				printf("MNP ERROR: unsupported frame type %d.\r\n", c);
				in_state_ = InState::ABORT;
				break;
			}
			break;
		case InState::WAIT_FOR_HDR_DATA:
			if (c == 0x10) {
				if (in_frame->escaping_dle) {
					in_frame->escaping_dle = false;
					// fall through
				} else {
					in_frame->escaping_dle = true;
					break;
				}
			}
			if (in_frame->escaping_dle) {
				printf("MNP ERROR: invalid DLE escape 0x10 0x%02x\r\n", c);
				in_state_ = InState::ABORT;
			}
			in_frame->header.push_back(c);
			if (in_frame->header.size() == in_frame->header_size) {
				in_state_ = InState::WAIT_FOR_DATA;
			}
			break;

		// -- read data until we reach the 0x10 0x03
		case InState::WAIT_FOR_DATA:
			if (c == 0x10) in_state_ = InState::WAIT_FOR_ETX;
			else in_frame->data.push_back(c); // TODO: check for overflow
			break;

		// -- read the end of the block
		case InState::WAIT_FOR_ETX:
			if (c == 0x10) {
				in_frame->data.push_back(c); // TODO: check for overflow
				in_state_ = InState::WAIT_FOR_DATA;
			} else if (c == 0x03) {
				in_state_ = InState::WAIT_FOR_CRC_lo;
			} else {
				in_state_ = InState::ABORT; // abort
			}
			break;

		// -- read the CRC	
		case InState::WAIT_FOR_CRC_lo:
			in_frame->crc = c;
			in_state_ = InState::WAIT_FOR_CRC_hi;
			break;
		case InState::WAIT_FOR_CRC_hi:
			in_frame->crc |= (c << 8);
			my_crc = in_frame->calculate_crc();
			if (my_crc != in_frame->crc) {
				Log.log("MNP in CRC error\r\n");
				printf("MNP ERROR: CRC error 0x%04x != 0x%04x\r\n", in_frame->crc, my_crc);
				in_state_ = InState::ABORT;
			} else {
				Log.log("MNP in CRC VALID\r\n");
				in_frame->crc_valid = true;
				handle_valid_in_frame(in_frame);
			}
			in_state_ = InState::WAIT_FOR_SYN;
			// TODO: check the CRC. If it is invalid, ignore the entire block
			// TODO: manage LR blocks (the other side wants to establish a connection)
			// TODO: manage LD blocks (the other side wants to disconnect)
			// TODO: manage LA blocks (see if our last block was receive without errors)
			// TODO: manage LT blocks and forward the data in them to our client (Dock)
			// NOTE: we don't have much time here at all
			// NOTE: if we set the in_state_ to WAIT_FOR_SYN, our in_* data may be immediatly overwritten again
			// We could have multiple in_* structs and activate them one after the other
			// We could also instruct the serial port to set the hardware handshake to stop the data stream
			in_state_ = InState::WAIT_FOR_SYN;
			break;
	}
	return Result::OK; // Just signal that we processed the byte.
}

Result PicoDock::signal(Event event) {
	return Endpoint::signal(event);
}

/**
 * \brief Handle a valid incomming MNP frame.
 * This is called when the CRC of the incomming frame is valid.
 * We need to check the type of the frame and handle it accordingly.
 */
void PicoDock::handle_valid_in_frame(Frame *frame) {
	// frame->print();
	switch (frame->type) {
		case kMNP_Frame_LR: {
			// Handle Link Request
			// This can happen even if we think we are in connected state, so just renegotiate.
			// - reply with our LR frame to "negotiate"
			// - set the mnp state to connection pending
			if (mnp_state_ != MNPState::DISCONNECTED) set_disconnected();
			set_negotiating();
			Log.log("\r\n**** MNP Negotiating\r\n");
			reply_to_LR(frame);
			break;
		}
		case kMNP_Frame_LD: {
			// Handle Link Disconnect
			// - do we need to acknowledge this (LA)?
			// - set the mnp state to disconnected
			set_disconnected();
			Log.log("\r\n**** MNP Disconnected\r\n");
			// TODO: we can print the reason for disconnecting
			break;
		}
		case kMNP_Frame_LT: {
			// Handle Link Transfer
			// Input Pipe set us data. Check if the sequence number is correct.
			// - if so, send an LA frame to acknowledge the data and forward it to the Dock
			// - if not, send an LA frame with the last sequence number we received
			if (frame->header_size == 2) {
				// TODO: this assumes that the previous frame was received correctly.
				// We should only set the new seq# if it is the old seq# plus 1.
				in_seq_no_ = frame->header[1];
			} else if (frame->header_size == 4) {
				// TODO: this assumes that the previous frame was received correctly.
				// We should only set the new seq# if it is the old seq# plus 1.
				in_seq_no_ = frame->header[3];
			} else {
				printf("MNP ERROR: Unexpected LT header size %d\r\n", frame->header_size);	
			}
			if (mnp_state_ == MNPState::CONNECTED) {
				Log.log("\r\n**** MNP Data Packet\r\n");
				reply_with_LA();
				handle_LT(frame);
			} else {
				Log.log("\r\n**** ERROR: MNP Data Packet while not connected\r\n");
				reply_with_LD();
			}
			break;
		}
		case kMNP_Frame_LA: {
			// Handle Link Acknowledgement
			// If the last Frame we sent was a Link Request, check the LA and 
			// set the mnp state to connected or disconnected.
			// If the last Frame we sent was a Link Transfer, check the LA.
			// If the sequence number is correct, we can send the next frame.
			// If the sequence number is not correct, we need to start resending
			// frames from the last acknowledged sequence number.
			// 
			// >03. >05. >xx. >yy. or 07 05 01 01 xx 02 01 yy
			// xx = sequence, yy = credit
			//
			uint8_t seq = 0, credit = 0;
			if (frame->header_size == 3) {
				seq = frame->header[1];
				credit = frame->header[2];
			} else if (frame->header_size == 7) {
				seq = frame->header[3];
				credit = frame->header[6];
			} else {
				printf("MNP ERROR: Unexpected LA header size %d\r\n", frame->header_size);	
			}
			if (mnp_state_ == MNPState::CONNECTED) {
				// TODO: is there a Frame that we need to resend?
				// TODO: also, is there a timeout with no LA?
			} else if (mnp_state_ == MNPState::NEGOTIATING) {
				// TODO: add a timeout to an earlier LR with no LA!
				set_connected();
				out_seq_no_ = seq;
				Log.log("\r\n**** MNP Connected\r\n");
			} else {
			}
			break;
		}
		default:
			break;
	}
}

void PicoDock::handle_LT(Frame *frame) {
	frame->print();
	if ((frame->data.size() >= 12) && (strncmp((const char*)frame->data.data(), "newtdock", 8) == 0)) {
		char buf[5];
		uint32_t cmd; memcpy(&cmd, (const char*)frame->data.data() + 8, 4);
		strncpy(buf, (const char*)frame->data.data() + 8, 4);
		buf[4] = 0;
		Log.log("\r\nNewton Dock command: ");
		Log.log(buf);
		Log.log("\r\n");
		if (cmd == ND_FOURCC('r','t','d','k')) {
			reply_newtdockdock();
		}
	} else {
		Log.log("Newton Dock: no signature found\r\n");
	}
}

void PicoDock::reply_to_LR(Frame *frame) {
	Frame *lr = new Frame();
	lr->header = { 
		kMNP_Frame_LR, 
		      0x02, 0x01, 0x06, 0x01, 0x00, 0x00, 0x00,
		0x00, 0xFF, 0x02, 0x01, 0x02, 0x03, 0x01, 0x01, 
		0x04, 0x02, 0x40, 0x00, 0x08, 0x01, 0x03
	};
	lr->prepare_to_send();
	// TODO: enqueue the frame to the outgoing pipe
	out_state_ = OutState::SEND_SYN;
    out_frame = lr;
	// frame->print();
	// lr->print();
}

void PicoDock::reply_with_LD() {
	// 1 Protocol establishment phase error, LR expected but not received
	// 2 LR constant parameter 1 contains an unexpected value
	// 3 LR received with incompatible or unknown parameter value
	// 255 User-initiated disconnect
	Frame *ld = new Frame();
	ld->header.push_back(kMNP_Frame_LD);
	ld->header.push_back(1);
	ld->header.push_back(1);
	ld->header.push_back(255); // reason
	ld->prepare_to_send();
	// TODO: enqueue the frame to the outgoing pipe
	out_state_ = OutState::SEND_SYN;
    out_frame = ld;
}

void PicoDock::reply_with_LA() {
	Frame *la = new Frame();
	la->header.push_back(kMNP_Frame_LA);
	la->header.push_back(in_seq_no_);
	la->header.push_back(1); // TODO: credit
	la->prepare_to_send();
	// TODO: enqueue the frame to the outgoing pipe
	out_state_ = OutState::SEND_SYN;
    out_frame = la;
}

void PicoDock::reply_newtdockdock() {
	Log.log("MNP: Pushing newtdockdock\r\n");
	// reply with <16. <10. <02. <02. <04. <01. "newtdockdock" <00. <00. <00. <04. <00. <00. <00. <01. <10. <03. <BA. <4FO
	// 'newt', 'dock', 'dock', 4, 1
	Frame *lt = new Frame();
	out_seq_no_++;
	lt->header.push_back(kMNP_Frame_LT);
	lt->header.push_back(out_seq_no_);
	lt->data = { 
		0x6e, 0x65, 0x77, 0x74, 0x64, 0x6f, 0x63, 0x6b,
		0x64, 0x6f, 0x63, 0x6b, 0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0x01
	};
	lt->prepare_to_send();
	// TODO: enqueue the frame to the outgoing pipe
	out_state_ = OutState::SEND_SYN;
    out_frame = lt;
}

void PicoDock::set_disconnected() {
	// TODO: free up all in and out frames
	// TODO: clear the in and out jobs queue
	mnp_state_ = MNPState::DISCONNECTED;
	in_seq_no_ = 0;
	out_seq_no_ = 0;
}

void PicoDock::set_negotiating() {
	// TODO: the next LA or LD frame will change our state
	// TODO: no other frames are allowed
	mnp_state_ = MNPState::NEGOTIATING;
}

void PicoDock::set_connected() {
	// TODO: we are connected, we can send and receive data
	mnp_state_ = MNPState::CONNECTED;
}





#if 0

/* Structure definition */
#define uchar unsigned char
#define ushort unsigned short
typedef struct {
	char devName[256];
	int speed;
	} NewtDevInfo;

/* Constants definition */
#define MaxHeadLen 256
#define MaxInfoLen 256
extern uchar LRFrame[];
extern uchar LDFrame[];

/* Function prototype */
int InitNewtDev(NewtDevInfo *newtDevInfo);
void FCSCalc(ushort *fcsWord, uchar octet);
void SendFrame(int newtFd, uchar *head, uchar *info, int infoLen);
void SendLTFrame(int newtFd, uchar seqNo, uchar *info, int infoLen);
void SendLAFrame(int newtFd, uchar seqNo);
int RecvFrame(int newtFd, uchar *frame);
int WaitLAFrame(int newtFd, uchar seqNo);
int WaitLDFrame(int newtFd);
void ErrHandler(char *errMesg);
void SigInt(int sigNo);

/* UnixNPI */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "newtmnp.h"

#define VERSION "1.1.5"

#define TimeOut 30

/* Function prototype */
void SigAlrm(int sigNo);

#define NUM_STARS 32

static char* stars(int number) {
	static char string[NUM_STARS+1];
	int i;
	for(i=0; i<NUM_STARS; i++) {
		if(i<number) string[i]='*';
		else string[i]=' ';
	}
	string[NUM_STARS]='\0';
	return string;
}

int main(int argc, char *argv[])
{
	FILE *inFile;
	long inFileLen;
	long tmpFileLen;
	uchar sendBuf[MaxHeadLen + MaxInfoLen];
	uchar recvBuf[MaxHeadLen + MaxInfoLen];
	NewtDevInfo newtDevInfo;
	int newtFd;
	uchar ltSeqNo;
	int i, j, k;

	k = 1;

	/* Initialization */
	fprintf(stdout, "\n");
	fprintf(stdout, "UnixNPI - a Newton Package Installer for Unix platforms\n");
	fprintf(stdout, "Version " VERSION " by Richard C. L. Li, Victor Rehorst, Chayam Kirshen\n");
	fprintf(stdout, "patches by Phil <phil@squack.COM>, Heinrik Lipka\n");
	fprintf(stdout, "This program is distributed under the terms of the GNU GPL: see the file COPYING\n");

	strcpy(newtDevInfo.devName, "/dev/newton");

	/* Install time out function */
	if(signal(SIGALRM, SigAlrm) == SIG_ERR)
		ErrHandler("Error in setting up timeout function!!");

	/* Check arguments */
	if(argc < 2)
		ErrHandler("Usage: unixnpi [-s speed] [-d device] PkgFiles...");
	else
	{
		if (strcmp(argv[1],"-s") == 0)
		{
			if (argc < 4)
				ErrHandler("Usage: unixnpi [-s speed] [-d device] PkgFiles...");
			newtDevInfo.speed = atoi(argv[2]);
			k = 3;
		}
		else
		{
			newtDevInfo.speed = 38400;
			k = 1;
		}
		if (strcmp(argv[k],"-d")==0)
		{
			if (argc<(k+1))
				ErrHandler("Usage: unixnpi [-s speed] [-d device] PkgFiles...");
			strcpy(newtDevInfo.devName, argv[k+1]);
			k+=2;
		}
	}

	/* Initialize Newton device */
	if((newtFd = InitNewtDev(&newtDevInfo)) < 0)
		ErrHandler("Error in opening Newton device!!\nDo you have a symlink to /dev/newton?");
	ltSeqNo = 0;

	/* Waiting to connect */
	fprintf(stdout, "\nWaiting to connect\n");
	do {
		while(RecvFrame(newtFd, recvBuf) < 0);
	} while(recvBuf[1] != '\x01');
	fprintf(stdout, "Connected\n");
	fprintf(stdout, "Handshaking");
	fflush(stdout);

	/* Send LR frame */
	alarm(TimeOut);
	do {
		SendFrame(newtFd, LRFrame, NULL, 0);
		} while(WaitLAFrame(newtFd, ltSeqNo) < 0);
	ltSeqNo++;
	fprintf(stdout, ".");
	fflush(stdout);

	/* Wait LT frame newtdockrtdk */
	while(RecvFrame(newtFd, recvBuf) < 0 || recvBuf[1] != '\x04');
	SendLAFrame(newtFd, recvBuf[2]);
	fprintf(stdout, ".");
	fflush(stdout);

	/* Send LT frame newtdockdock */
	alarm(TimeOut);
	do {
		SendLTFrame(newtFd, ltSeqNo, "newtdockdock\0\0\0\4\0\0\0\4", 20);
	} while(WaitLAFrame(newtFd, ltSeqNo) < 0);
	ltSeqNo++;
	fprintf(stdout, ".");
	fflush(stdout);

	/* Wait LT frame newtdockname */
	alarm(TimeOut);
	while(RecvFrame(newtFd, recvBuf) < 0 || recvBuf[1] != '\x04');
	SendLAFrame(newtFd, recvBuf[2]);
	fprintf(stdout, ".");
	fflush(stdout);

	/* Get owner name */
	i = recvBuf[19] * 256 * 256 * 256 + recvBuf[20] * 256 * 256 + recvBuf[21] *
		256 + recvBuf[22];
	i += 24;
	j = 0;
	while(recvBuf[i] != '\0') {
		sendBuf[j] = recvBuf[i];
		j++;
		i += 2;
		}
	sendBuf[j] = '\0';

	/* Send LT frame newtdockstim */
	alarm(TimeOut);
	do {
		SendLTFrame(newtFd, ltSeqNo, "newtdockstim\0\0\0\4\0\0\0\x1e", 20);
	} while(WaitLAFrame(newtFd, ltSeqNo) < 0);
	ltSeqNo++;
	fprintf(stdout, ".");
	fflush(stdout);

	/* Wait LT frame newtdockdres */
	alarm(TimeOut);
	while(RecvFrame(newtFd, recvBuf) < 0 || recvBuf[1] != '\x04');
	SendLAFrame(newtFd, recvBuf[2]);
	fprintf(stdout, ".\n");
	fflush(stdout);

	/* batch install all of the files */
	for (k; k < argc; k++)
	{
		/* load the file */
		if((inFile = fopen(argv[k], "rb")) == NULL)
			ErrHandler("Error in opening package file!!");
		fseek(inFile, 0, SEEK_END);
		inFileLen = ftell(inFile);
		rewind(inFile);
		/* printf("File is '%s'\n", argv[k]); */

		/* Send LT frame newtdocklpkg */
		alarm(TimeOut);
		strcpy(sendBuf, "newtdocklpkg");
		tmpFileLen = inFileLen;
		for(i = 15; i >= 12; i--) {
			sendBuf[i] = tmpFileLen % 256;
			tmpFileLen /= 256;
			}
		do {
			SendLTFrame(newtFd, ltSeqNo, sendBuf, 16);
		} while(WaitLAFrame(newtFd, ltSeqNo) < 0);
		ltSeqNo++;
		/* fprintf(stdout, ".\n"); */

		/* fprintf(stdout, "Sending %d / %d\r", 0, inFileLen); */
		fprintf(stdout, "%20s %3d%% |%s| %d\r",
			argv[k], 0, stars(0), 0);
		fflush(stdout);

		/* Send package data */
		while(!feof(inFile)) {
                        int bytes;
			alarm(TimeOut);
			i = fread(sendBuf, sizeof(uchar), MaxInfoLen, inFile);
			while(i % 4 != 0)
				sendBuf[i++] = '\0';
			do {
				SendLTFrame(newtFd, ltSeqNo, sendBuf, i);
			} while(WaitLAFrame(newtFd, ltSeqNo) < 0);
			ltSeqNo++;
			if(ltSeqNo % 4 == 0) {
				/* fprintf(stdout, "Sending %d / %d\r", ftell(inFile), inFileLen); */
				bytes=ftell(inFile);
				fprintf(stdout, "%20s %3d%% |%s| %d\r",
					argv[k],
					(int)(((float)bytes/(float)inFileLen)*100),
					stars(((float)bytes/(float)inFileLen)*NUM_STARS),
					bytes);
				fflush(stdout);
			}
		}
		/* fprintf(stdout, "Sending %d / %d\n", inFileLen, inFileLen); */
		fprintf(stdout, "\n");

		/* Wait LT frame newtdockdres */
		alarm(TimeOut);
		while(RecvFrame(newtFd, recvBuf) < 0 || recvBuf[1] != '\x04');
		SendLAFrame(newtFd, recvBuf[2]);

		fclose (inFile);

	} /* END OF FOR LOOP */

	/* Send LT frame newtdockdisc */
	alarm(TimeOut);
	do {
		SendLTFrame(newtFd, ltSeqNo, "newtdockdisc\0\0\0\0", 16);
	} while(WaitLAFrame(newtFd, ltSeqNo) < 0);

	/* Wait disconnect */
	alarm(0);
	WaitLDFrame(newtFd);
	fprintf(stdout, "Finished!!\n\n");

	/* fclose(inFile); */
	close(newtFd);
	return 0;
}

void SigAlrm(int sigNo)
{
	ErrHandler("Timeout error, connection stopped!!");
}
  


#include <stdio.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include "newtmnp.h"

/* Constants definition */
uchar FrameStart[] = "\x16\x10\x02";
uchar FrameEnd[] = "\x10\x03";
uchar LRFrame[] = {
	'\x17', /* Length of header */
	'\x01', /* Type indication LR frame */
	'\x02', /* Constant parameter 1 */
	'\x01', '\x06', '\x01', '\x00', '\x00', '\x00', '\x00', '\xff',
		/* Constant parameter 2 */
	'\x02', '\x01', '\x02', /* Octet-oriented framing mode */
	'\x03', '\x01', '\x01', /* k = 1 */
	'\x04', '\x02', '\x40', '\x00', /* N401 = 64 */
	'\x08', '\x01', '\x03' /* N401 = 256 & fixed LT, LA frames */
	};
uchar LDFrame[] = {
	'\x04', /* Length of header */
	'\x02', /* Type indication LD frame */
	'\x01', '\x01', '\xff'
	};

int intNewtFd;

int InitNewtDev(NewtDevInfo *newtDevInfo)
{
	int newtFd;
	struct termios newtTty;

	/*  Install Ctrl-C function */
	intNewtFd = -1;
	if(signal(SIGINT, SigInt) == SIG_ERR)
		ErrHandler("Error in setting up interrupt function!!");

	/* Open the Newton device */
	if((newtFd = open(newtDevInfo->devName, O_RDWR)) == -1)
		return -1;

	/* Get the current device settings */
	tcgetattr(newtFd, &newtTty);

	/* Change the device settings */
	newtTty.c_iflag = IGNBRK | INPCK;
	newtTty.c_oflag = 0;
	newtTty.c_cflag = CREAD | CLOCAL | CS8 & ~PARENB & ~PARODD & ~CSTOPB;
	newtTty.c_lflag = 0;
	newtTty.c_cc[VMIN] = 1;
	newtTty.c_cc[VTIME] = 0;

	/* Select the communication speed */
	switch(newtDevInfo->speed) {
		#ifdef B2400
		case 2400:
			cfsetospeed(&newtTty, B2400);
			cfsetispeed(&newtTty, B2400);
			break;
		#endif
		#ifdef B4800
		case 4800:
			cfsetospeed(&newtTty, B4800);
			cfsetispeed(&newtTty, B4800);
			break;
		#endif
		#ifdef B9600
		case 9600:
			cfsetospeed(&newtTty, B9600);
			cfsetispeed(&newtTty, B9600);
			break;
		#endif
		#ifdef B19200
		case 19200:
			cfsetospeed(&newtTty, B19200);
			cfsetispeed(&newtTty, B19200);
			break;
		#endif
		#ifdef B38400
		case 38400:
			cfsetospeed(&newtTty, B38400);
			cfsetispeed(&newtTty, B38400);
			break;
		#endif
		#ifdef B57600
		case 57600:
			cfsetospeed(&newtTty, B57600);
			cfsetispeed(&newtTty, B57600);
			break;
		#endif
		#ifdef B115200
		case 115200:
			cfsetospeed(&newtTty, B115200);
			cfsetispeed(&newtTty, B115200);
			break;
		#endif
		#ifdef B230400
		case 230400:
			cfsetospeed(&newtTty, B230400);
			cfsetispeed(&newtTty, B230400);
			break;
		#endif
		default:
			close(newtFd);
			return -1;
		}

	/* Flush the device and restarts input and output */
	tcflush(newtFd, TCIOFLUSH);
	tcflow(newtFd, TCOON);

	/* Update the new device settings */
	tcsetattr(newtFd, TCSANOW, &newtTty);

	/* Return with file descriptor */
	intNewtFd = newtFd;
	return newtFd;
}

void FCSCalc(ushort *fcsWord, unsigned char octet)
{
	int i;
	uchar pow;

	pow = 1;
	for(i = 0; i < 8; i++) {
		if((((*fcsWord % 256) & 0x01) == 0x01) ^ ((octet & pow) == pow))
			*fcsWord = (*fcsWord / 2) ^ 0xa001;
		else
			*fcsWord /= 2;
		pow *= 2;
		}
}

void SendFrame(int newtFd, uchar *head, uchar *info, int infoLen)
{
	char errMesg[] = "Error in writing to Newton device, connection stopped!!";
	int i;
	ushort fcsWord;
	uchar buf;

	/* Initialize */
	fcsWord = 0;

	/* Send frame start */
	if(write(newtFd, FrameStart, 3) < 0)
		ErrHandler(errMesg);

	/* Send frame head */
	for(i = 0; i <= head[0]; i++) {
		FCSCalc(&fcsWord, head[i]);
		if(write(newtFd, &head[i], 1) < 0)
			ErrHandler(errMesg);
		if(head[i] == FrameEnd[0]) {
			if(write(newtFd, &head[i], 1) < 0)
				ErrHandler(errMesg);
			}
		}

	/* Send frame information */
	if(info != NULL) {
		for(i = 0; i < infoLen; i++) {
			FCSCalc(&fcsWord, info[i]);
			if(write(newtFd, &info[i], 1) < 0)
				ErrHandler(errMesg);
			if(info[i] == FrameEnd[0]) {
				if(write(newtFd, &info[i], 1) < 0)
					ErrHandler(errMesg);
				}
			}
		}

	/* Send frame end */
	if(write(newtFd, FrameEnd, 2) < 0)
		ErrHandler(errMesg);
	FCSCalc(&fcsWord, FrameEnd[1]);

	/* Send FCS */
	buf = fcsWord % 256;
	if(write(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);
	buf = fcsWord / 256;
	if(write(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);

	return;
}

void SendLTFrame(int newtFd, uchar seqNo, uchar *info, int infoLen)
{
	uchar ltFrameHead[3] = {
		'\x02', /* Length of header */
		'\x04', /* Type indication LT frame */
		};

	ltFrameHead[2] = seqNo;
	SendFrame(newtFd, ltFrameHead, info, infoLen);

	return;
}

void SendLAFrame(int newtFd, uchar seqNo)
{
	uchar laFrameHead[4] = {
		'\x03', /* Length of header */
		'\x05', /* Type indication LA frame */
		'\x00', /* Sequence number */
		'\x01' /* N(k) = 1 */
		};

	laFrameHead[2] = seqNo;
	SendFrame(newtFd, laFrameHead, NULL, 0);

	return;
}

int RecvFrame(int newtFd, unsigned char *frame)
{
	char errMesg[] = "Error in reading from Newton device, connection stopped!!";
	int state;
	unsigned char buf;
	unsigned short fcsWord;
	int i;

	/* Initialize */
	fcsWord = 0;
	i = 0;

	/* Wait for head */
	state = 0;
	while(state < 3) {
		if(read(newtFd, &buf, 1) < 0)
			ErrHandler(errMesg);
    //printf("Received %02x\n", buf);
		switch(state) {
			case 0:
				if(buf == FrameStart[0])
					state++;
				break;
			case 1:
				if(buf == FrameStart[1])
					state++;
				else
					state = 0;
				break;
			case 2:
				if(buf == FrameStart[2])
					state++;
				else
					state = 0;
				break;
			}
		}

	/* Wait for tail */
	state = 0;
	while(state < 2) {
		if(read(newtFd, &buf, 1) < 0)
			ErrHandler(errMesg);
		switch(state) {
			case 0:
				if(buf == '\x10')
					state++;
				else {
					FCSCalc(&fcsWord, buf);
					if(i < MaxHeadLen + MaxInfoLen) {
						frame[i] = buf;
						i++;
						}
					else
						return -1;
					}
				break;
			case 1:
				if(buf == '\x10') {
					FCSCalc(&fcsWord, buf);
					if(i < MaxHeadLen + MaxInfoLen) {
						frame[i] = buf;
						i++;
						}
					else
						return -1;
					state = 0;
					}
				else {
					if(buf == '\x03') {
						FCSCalc(&fcsWord, buf);
						state++;
						}
					else
						return -1;
					}
				break;
			}
		}

	/* Check FCS */
	if(read(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);
	if(fcsWord % 256 != buf)
		return -1;
	if(read(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);
	if(fcsWord / 256 != buf)
		return -1;

	if(frame[1] == '\x02')
		ErrHandler("Newton device disconnected, connection stopped!!");
	return 0;
}

int WaitLAFrame(int newtFd, uchar seqNo)
{
	uchar frame[MaxHeadLen + MaxInfoLen];

	do {
		while(RecvFrame(newtFd, frame) < 0);
		if(frame[1] == '\x04')
			SendLAFrame(newtFd, frame[2]);
		} while(frame[1] != '\x05');
	if(frame[2] == seqNo)
		return 0;
	else
		return -1;
}

int WaitLDFrame(int newtFd)
{
	char errMesg[] = "Error in reading from Newton device, connection stopped!!";
	int state;
	unsigned char buf;
	unsigned short fcsWord;
	int i;

	/* Initialize */
	fcsWord = 0;
	i = 0;

	/* Wait for head */
	state = 0;
	while(state < 5) {
		if(read(newtFd, &buf, 1) < 0)
			ErrHandler(errMesg);
		switch(state) {
			case 0:
				if(buf == FrameStart[0])
					state++;
				break;
			case 1:
				if(buf == FrameStart[1])
					state++;
				else
					state = 0;
				break;
			case 2:
				if(buf == FrameStart[2])
					state++;
				else
					state = 0;
				break;
			case 3:
				FCSCalc(&fcsWord, buf);
				state++;
				break;
			case 4:
				if(buf == '\x02') {
					FCSCalc(&fcsWord, buf);
					state++;
					}
				else {
					state = 0;
					fcsWord = 0;
					}
				break;
			}
		}

	/* Wait for tail */
	state = 0;
	while(state < 2) {
		if(read(newtFd, &buf, 1) < 0)
			ErrHandler(errMesg);
		switch(state) {
			case 0:
				if(buf == '\x10')
					state++;
				else
					FCSCalc(&fcsWord, buf);
				break;
			case 1:
				if(buf == '\x10') {
					FCSCalc(&fcsWord, buf);
					state = 0;
					}
				else {
					if(buf == '\x03') {
						FCSCalc(&fcsWord, buf);
						state++;
						}
					else
						return -1;
					}
				break;
			}
		}

	/* Check FCS */
	if(read(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);
	if(fcsWord % 256 != buf)
		return -1;
	if(read(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);
	if(fcsWord / 256 != buf)
		return -1;

	return 0;
}

void ErrHandler(char *errMesg)
{
	fprintf(stderr, "\n");
	fprintf(stderr, errMesg);
	fprintf(stderr, "\n\n");
	_exit(0);
}

void SigInt(int sigNo)
{
	if(intNewtFd >= 0) {
		/* Wait for all buffer sent */
		tcdrain(intNewtFd);

		SendFrame(intNewtFd, LDFrame, NULL, 0);
		}
	ErrHandler("User interrupted, connection stopped!!");
}


#endif
