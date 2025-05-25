//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "common/Filters/MNPFilter.h"

#include "main.h"

#include <stdio.h>
#include <cstring>

using namespace nd; 

/**
 * \class nd::MNPFilter
 * \brief MNPFilter connects a downstream MNP data source to an upstream data stream.
 * 
 * MNP is an error correction protocol used by modems over insecure lines. It
 * converts a binary data instream into a series of frames that are verified
 * for integrity and correctness, and resent if necessary.
 * 
 * In this module, the Newton sends its docking protocol over the serial port
 * encoded in MNP. This filter decodes the MNP frames and sends the binary
 * data to the upstream Dock. It takes binary data from the Dock, encodes it 
 * into MNP frames, and sends it downstream to the Newton.
 * 
 * The MNP filter is completely agnostic to the actual data being sent.
 * It only cares about the MNP protocol and the integrity of the data.
 * 
 * Clients of the MNP Filter need not worry about block sizes. They can just 
 * stream data in and out.
 */

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

/* Generate CRC16 checksum. Note that this is different than the FatFS CRC16 calculation. */
static void update_crc16(unsigned short *pCrc16, const char data[], size_t length) {
	for (size_t i = 0; i < length; i++) {
		uint16_t in_crc = *pCrc16;
		*pCrc16 = (uint16_t)(((in_crc >> 8) & 0xFF) ^ kCRC16Table[ (in_crc & 0xFF) ^ data[i] ]);
	}
}

/**
 * \brief Allocate a universal MNP frame.
 * All frames are held in a small pool inside MNPFilter.
 */
MNPFilter::Frame::Frame() 
{
	header.reserve(256);
	data.reserve(1024);
}

/**
 * \brief Clear the frame, but don't change `in_use`.
 */
void MNPFilter::Frame::clear() {
	header.clear();
	data.clear();
	header_size = 0;
	crc = 0;
	type = 0;
	escaping_dle = false;
	crc_valid = false;
	// don't change `in_use`!
}

/**
 * \brief Calculate the CRC16 checksum for this frame.
 * The CRC is calculated over the size, header, data, and ETX.
 * This does not store the CRC in the frame.
 */
uint16_t MNPFilter::Frame::calculate_crc() {
	static uint8_t ETX = 0x03;
	uint16_t my_crc = 0;
	uint8_t hsze = header.size();
	update_crc16(&my_crc, (const char*)&hsze, 1);
	update_crc16(&my_crc, (const char*)header.data(), header.size());
	update_crc16(&my_crc, (const char*)data.data(), data.size());
	update_crc16(&my_crc, (const char*)&ETX, 1);
	return my_crc;
}

/**
 * \brief After header and data are filled in, prepare the frame for sending.
 * This will calculate the CRC and set the type.
 * \todo Please don't duplicate values in header_size and header.size(),
 *       and type and header[0].
 */
void MNPFilter::Frame::prepare_to_send() {
	header_size = header.size();
	type = header[0];
	crc = calculate_crc();
	crc_valid = true;
}

/**
 * \brief Print the contents of the frame.
 */
void MNPFilter::Frame::print() {
	Log.log("--- Frame ---\r\n");
	switch (type) {
		case kMNP_Frame_LR: Log.log("Link Request\r\n"); break;
		case kMNP_Frame_LD: Log.log("Link Disconnect\r\n"); break;
		case kMNP_Frame_LT: Log.log("Link Transfer\r\n"); break;
		case kMNP_Frame_LA: Log.log("Link Acknowledge\r\n"); break;
		default: Log.log("Unknown Frame Type %d\r\n", type); break;
	}
	Log.log("Header: ");
	for (auto c : header) {
		Log.logf("%02x ", c);
	}
	Log.log("\r\n");
	Log.log("Data: ");
	for (auto c : data) {
		Log.logf("%02x ", c);
	}
	Log.log("\r\n");
	if (crc_valid) {
		Log.logf("CRC: %04x\r\n", crc);
	} else {
		Log.log("CRC: invalid\r\n");
	}
	Log.log("--- Frame End ---\r\n");
}

// =============================================================================

/**
 * \brief Create a new MNPFilter object.
 * \todo This will be renamed to MNPFilter or similar
 * \todo This should not be an Endpoint, but a filter that forwards the decoded
 *    MNP data as a stream down the pipe to a Dock class.
 */
MNPFilter::MNPFilter(Scheduler &scheduler) 
:   super(scheduler)
{
	uint32_t i;
	for (i=0; i<kPoolSize; i++) {
		in_pool[i].pool_index = i;
		out_pool[i].pool_index = i;
	}
    in_frame = acquire_in_frame();
}

/**
 * \brief Handle out pipes and timeouts.
 */
Result MNPFilter::task() {
	// TODO: `send()` will receive LA acknowledgements and may request that we send
	// a frame again if it did not get acknowledged or timed out.

	if (dock_job_list.size() > 0) {
		Event event = dock_job_list.front();
		if (event.type() != Event::Type::MNP) {
			if (kLogMNPErrors) Log.log("MNP Error: dock job type not MNP\r\n");
			dock_job_list.pop();
		} else {
			switch (event.subtype()) {
				case Event::Subtype::MNP_DATA_TO_DOCK: // data() is index in in_pool
                    if (!dock_out_frame) {
                        dock_out_frame = &in_pool[event.data()];
                        dock_out_index_ = 0;
                        dock_job_list.pop();
                    } 
					break;
				default:
					if (kLogMNPErrors) Log.log("MNP Error: unknown dock job\r\n");
					dock_job_list.pop();
					break;
			}
		}
	}
    if (dock_out_frame) {
        // Continue to send the current frame to "dock".
        if (dock_out_index_ == 0) {
            // TODO: does not handle rejects
            dock.out()->send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_START));
        }
        if (dock_out_index_ < dock_out_frame->data.size()) {
            Pipe *o = dock.out();
            if (o && o->send(dock_out_frame->data[dock_out_index_]).ok()) {
                dock_out_index_++;
            }
        } 
        if (dock_out_index_ == dock_out_frame->data.size()) {
            // Finished sending the frame, release it.
            release_in_frame(dock_out_frame);
            // TODO: does not handle rejects
            dock.out()->send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_END));
            dock_out_frame = nullptr;
        }
    }   

	// TODO: handle various timeouts and trigger retries or disconnects.

	if (out_frame) {
		// Continue to send the current frame to "out".
		state_machine_reply(out_frame);
	} else if (newton_job_list.size() > 0) {
		Frame *my_frame = nullptr;
		Event event = newton_job_list.front();
		if (event.type() != Event::Type::MNP) {
			if (kLogMNPErrors) Log.log("MNP Error: job type not MNP\r\n");
			newton_job_list.pop();
		} else {
			switch (event.subtype()) {
				case Event::Subtype::MNP_SEND_LA: // data() is in sequence number
					my_frame = acquire_out_frame();
					if (!my_frame) {
						if (kLogMNPWarnings) Log.log("MNP_SEND_LA: no out frame available, try again.\r\n");
					} else {
						prepare_LA_frame(my_frame, event.data());
						out_frame = my_frame; // TODO: trigger sending state
						newton_job_list.pop();
					}
					break;
				case Event::Subtype::MNP_SEND_LD: // data() is disconnect reason
					my_frame = acquire_out_frame();
					if (!my_frame) {
						if (kLogMNPWarnings) Log.log("MNP_SEND_LD: no out frame available, try again.\r\n");
					} else {
						prepare_LD_frame(my_frame, event.data());
						out_frame = my_frame; // TODO: trigger sending state
						newton_job_list.pop();
					}
					break;
				case Event::Subtype::MNP_SEND_LR: // data() is original LR index on in_pool
					my_frame = acquire_out_frame();
					if (!my_frame) {
						if (kLogMNPWarnings) Log.log("MNP_SEND_LR: no out frame available, try again.\r\n");
					} else {
						prepare_LR_frame(my_frame, event.data());
						out_frame = my_frame; // TODO: trigger sending state
						newton_job_list.pop();
						Frame *in_frame = &in_pool[event.data()]; 
						release_in_frame(in_frame);
					}
					break;
				case Event::Subtype::MNP_SEND_LT: // data() is index in out_pool
					out_frame = &out_pool[event.data()];
					out_frame->prepare_to_send();
					out_state_ = OutState::SEND_SYN;
					newton_job_list.pop();
					break;
				default:
					if (kLogMNPErrors) Log.log("MNP Error: unknown job\r\n");
					newton_job_list.pop();
					break;
				}
		}
	} else {
		// idle.
	}
	return Result::OK;
}

/**
 * \brief Send out a frame toward the Newton in a state machine.
 * After the frame is sent, it will be released and the out_frame is set to
 * nullptr, allowing for the next frame to be queued.
 */
void MNPFilter::state_machine_reply(Frame *frame) {
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
				release_out_frame(frame);
				out_state_ = OutState::SEND_SYN;
				out_frame = nullptr;
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
Result MNPFilter::send(Event event) {
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
		return super::send(event);
	}

	// If we don't have an in_frame, try to get one from the pool.
	if (!in_frame) {
		in_frame = acquire_in_frame();
		if (!in_frame) {
			// No in frame is available. Tell the in pipe to resend the data again later.
			if (kLogMNPWarnings) Log.log("MNP Warning: `send` no in frame available\r\n");
			return Result::REJECTED;
		}
	}

	// At this point, in_frame is guaranteed to point to a valid frame.
	uint8_t c = event.data();
	switch (in_state_) {
		case InState::ABORT:
			// -- abort the current frame and start over
			in_frame->clear();
			in_state_ = InState::WAIT_FOR_SYN;
			[[fallthrough]];
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
				if (kLogMNPErrors) Log.logf("MNP ERROR: unsupported header size %d.\r\n", c);
				in_state_ = InState::ABORT;
				break;
			}
			in_frame->header_size = c;
			in_state_ = InState::WAIT_FOR_HDR_TYPE;
			break;
        case InState::WAIT_FOR_HDR_TYPE:
			in_frame->header.push_back(c);
			if (c == kMNP_Frame_LR) {
				if (kLogMNFlow) Log.log("(LR) ");
				in_frame->type = kMNP_Frame_LR;
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else if (c == kMNP_Frame_LD) {
				if (kLogMNFlow) Log.log("(LD) ");
				in_frame->type = kMNP_Frame_LD;
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else if (c == kMNP_Frame_LT) {
				if (kLogMNFlow) Log.log("(LT) ");
				in_frame->type = kMNP_Frame_LT;
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else if (c == kMNP_Frame_LA) {
				if (kLogMNFlow) Log.log("(LA) ");
				in_frame->type = kMNP_Frame_LA;
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else {
				if (kLogMNPErrors) Log.logf("\r\nMNP ERROR: unsupported frame type %d.\r\n", c);
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
				if (kLogMNPErrors) Log.logf("MNP ERROR: invalid DLE escape 0x10 0x%02x\r\n", c);
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
				if (kLogMNPErrors) Log.logf("MNP ERROR: CRC error 0x%04x != 0x%04x\r\n", in_frame->crc, my_crc);
				in_state_ = InState::ABORT;
			} else {
				in_frame->crc_valid = true;
				handle_valid_in_frame(in_frame);
				in_state_ = InState::WAIT_FOR_SYN;
				in_frame = nullptr; // this method must acquire a new frame, handle_valid_in_frame must release the frame
			}
			// TODO: check the CRC. If it is invalid, ignore the entire block
			// TODO: manage LR blocks (the other side wants to establish a connection)
			// TODO: manage LD blocks (the other side wants to disconnect)
			// TODO: manage LA blocks (see if our last block was receive without errors)
			// TODO: manage LT blocks and forward the data in them to our client (Dock)
			// NOTE: we don't have much time here at all
			// NOTE: if we set the in_state_ to WAIT_FOR_SYN, our in_* data may be immediatly overwritten again
			// We could have multiple in_* structs and activate them one after the other
			// We could also instruct the serial port to set the hardware handshake to stop the data stream
			break;
	}
	return Result::OK; // Just signal that we processed the byte.
}

Result MNPFilter::signal(Event event) {
	return super::signal(event);
}

void MNPFilter::flush_dock_in_frame() {
    Frame *f = dock_in_frame;
    dock_in_frame = nullptr;
    f->header.clear();
    f->header.push_back(kMNP_Frame_LT);
    f->header.push_back(++out_seq_no_);
    f->prepare_to_send();
    newton_job_list.push(Event(Event::Type::MNP, Event::Subtype::MNP_SEND_LT, f->pool_index));
}

Result MNPFilter::dock_send(Event event) {
    if (event.type() == Event::Type::MNP) {
        if ((event.subtype() == Event::Subtype::MNP_FRAME_START) || (event.subtype() == Event::Subtype::MNP_FRAME_END)){
            if (dock_in_frame) {
                if (kLogMNPErrors) Log.log("MNPFilter::dock_send: flush dock_in_frame on new frame\r\n");
                flush_dock_in_frame();
            }
            return Result::OK;
        }
    } else if (event.type() == Event::Type::DATA) {
        if (!dock_in_frame) {
            dock_in_frame = acquire_out_frame();
            if (!dock_in_frame) {
                if (kLogMNPWarnings) Log.log("MNPFilter::dock_send: no dock_in_frame available\r\n");
                return Result::REJECTED;
            }
        }
        if (kLogDock) Log.logf("+%02x ", event.data());
        dock_in_frame->data.push_back(event.data());
        if (dock_in_frame->data.size() >= kMaxData) {
            // We have enough data to send a LT frame.
            if (kLogDock) Log.log("\r\nDock: LT frame ready\r\n");
            flush_dock_in_frame();
        }
        return Result::OK;
	}
	if (kLogMNPErrors) Log.logf("MNPFilter::dock_send: unknown event type (%d:%d:%d)\r\n", (int)event.type(), (int)event.subtype(), (int)event.data());

	return Result::OK__NOT_HANDLED;
}

MNPFilter::Frame *MNPFilter::acquire_in_frame() {
	for (auto &frame : in_pool) {
		if (!frame.in_use) {
			frame.in_use = true;
			return &frame;
		}
	}
	return nullptr;
}

void MNPFilter::release_in_frame(Frame *frame) {
	if (frame) {
		frame->clear();
		frame->in_use = false;
	}
}

MNPFilter::Frame *MNPFilter::acquire_out_frame() {
	for (auto &frame : out_pool) {
		if (!frame.in_use) {
			frame.in_use = true;
			return &frame;
		}
	}
	return nullptr;
}

void MNPFilter::release_out_frame(Frame *frame) {
	if (frame) {
		frame->clear();
		frame->in_use = false;
	}
}

/**
 * \brief Handle a valid incomming MNP frame.
 * This is called when the CRC of the incomming frame is valid.
 * We need to check the type of the frame and handle it accordingly.
 */
void MNPFilter::handle_valid_in_frame(Frame *frame) {
	// frame->print();
	switch (frame->type) {
		case kMNP_Frame_LR: {
			// Handle Link Request
			// This can happen even if we think we are in connected state, so just renegotiate.
			// - reply with our LR frame to "negotiate"
			// - set the mnp state to connection pending
			if (mnp_state_ != MNPState::DISCONNECTED) set_disconnected();
			set_negotiating();
			if (kLogMNPState) Log.log("\r\n**** MNP Negotiating\r\n");
			newton_job_list.push(Event(Event::Type::MNP, Event::Subtype::MNP_SEND_LR, frame->pool_index));
            dock.out()->send(Event(Event::Type::MNP, Event::Subtype::MNP_NEGOTIATING));
			// NOTE: the LT handler must eventually release the in_frame
			break;
		}
		case kMNP_Frame_LD: {
			// Handle Link Disconnect
			// - do we need to acknowledge this (LA)?
			// - set the mnp state to disconnected
			set_disconnected();
			if (kLogMNPState) Log.log("\r\n**** MNP Disconnected\r\n");
			release_in_frame(frame);
            dock.out()->send(Event(Event::Type::MNP, Event::Subtype::MNP_DISCONNECTED));
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
				if (kLogMNPErrors) Log.logf("MNP ERROR: Unexpected LT header size %d\r\n", frame->header_size);
				release_in_frame(frame);
				break; // we ignore the frame because we can't get the sequence number
			}
			// TODO: we must verify that this is a valid sequence number
			if (mnp_state_ == MNPState::CONNECTED) {
				if (kLogMNPState) Log.log("\r\n**** MNP Data Packet\r\n");
				newton_job_list.push(Event(Event::Type::MNP, Event::Subtype::MNP_SEND_LA, in_seq_no_));
				dock_job_list.push(Event(Event::Type::MNP, Event::Subtype::MNP_DATA_TO_DOCK, frame->pool_index));
				// NOTE: the LT handler must eventually release the in_frame
			} else {
				if (kLogMNPErrors) Log.log("\r\n**** ERROR: MNP Data Packet while not connected\r\n");
				newton_job_list.push(Event(Event::Type::MNP, Event::Subtype::MNP_SEND_LD, 255));
				release_in_frame(frame);
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
				if (kLogMNPErrors) Log.logf("MNP ERROR: Unexpected LA header size %d\r\n", frame->header_size);	
			}
			if (mnp_state_ == MNPState::CONNECTED) {
				// TODO: is there a Frame that we need to resend?
				// TODO: also, is there a timeout with no LA?
			} else if (mnp_state_ == MNPState::NEGOTIATING) {
				// TODO: add a timeout to an earlier LR with no LA!
				set_connected();
				out_seq_no_ = seq;
				if (kLogMNPState) Log.log("\r\n**** MNP Connected\r\n");
                dock.out()->send(Event(Event::Type::MNP, Event::Subtype::MNP_CONNECTED));
			} else {
			}
			release_in_frame(frame);
			break;
		}
		default:
			release_in_frame(frame);
			if (kLogMNPErrors) Log.log("MNP ERROR: handle_valid_in_frame: Unknown frame type %d\r\n", frame->type);
			break;
	}
}


void MNPFilter::prepare_LR_frame(Frame *lr, uint8_t in_index) {
	lr->clear();
	lr->header = { 
		kMNP_Frame_LR, 
		      0x02, 0x01, 0x06, 0x01, 0x00, 0x00, 0x00,
		0x00, 0xFF, 0x02, 0x01, 0x02, 0x03, 0x01, 0x01, 
		0x04, 0x02, 0x40, 0x00, 0x08, 0x01, 0x03
	};
	lr->prepare_to_send();
	// TODO: enqueue the frame to the outgoing pipe
	out_state_ = OutState::SEND_SYN;
}

void MNPFilter::prepare_LD_frame(Frame *ld, uint8_t reason) {
	// 1 Protocol establishment phase error, LR expected but not received
	// 2 LR constant parameter 1 contains an unexpected value
	// 3 LR received with incompatible or unknown parameter value
	// 255 User-initiated disconnect
	ld->clear();
	ld->header.push_back(kMNP_Frame_LD);
	ld->header.push_back(1);
	ld->header.push_back(1);
	ld->header.push_back(reason);
	ld->prepare_to_send();
	// TODO: enqueue the frame to the outgoing pipe
	out_state_ = OutState::SEND_SYN;
}

void MNPFilter::prepare_LA_frame(Frame *la, uint8_t seq_no) {
	la->clear();
	la->header.push_back(kMNP_Frame_LA);
	la->header.push_back(seq_no);
	la->header.push_back(1); // TODO: credit
	la->prepare_to_send();
	// TODO: enqueue the frame to the outgoing pipe
	out_state_ = OutState::SEND_SYN;
}

void MNPFilter::set_disconnected() {
	// TODO: free up all in and out frames
	// TODO: clear the in and out jobs queue
	mnp_state_ = MNPState::DISCONNECTED;
	in_seq_no_ = 0;
	out_seq_no_ = 0;
}



void MNPFilter::set_negotiating() {
	// TODO: the next LA or LD frame will change our state
	// TODO: no other frames are allowed
	mnp_state_ = MNPState::NEGOTIATING;
}

void MNPFilter::set_connected() {
	// TODO: we are connected, we can send and receive data
	mnp_state_ = MNPState::CONNECTED;
	dock_state_ = 0;
}

