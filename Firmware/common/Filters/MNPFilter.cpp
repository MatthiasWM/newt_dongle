//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "common/Filters/MNPFilter.h"

#include "main.h"

#include <stdio.h>
#include <cstring>

using namespace nd; 

static void update_crc16(unsigned short *pCrc16, const char data[], size_t length);

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


 namespace nd {

class MNPFrame {
public:
    MNPFilter &filter_;
    std::vector<uint8_t> header;
    std::vector<uint8_t> data;
    uint16_t crc = 0;
    uint8_t expected_header_size = 0;
    uint8_t pool_index = 0;
    bool escaping_dle = false;
    bool crc_valid = false;
    bool in_use = false;

    MNPFrame(MNPFilter &filter);
    void clear();
    uint8_t type();
    uint16_t calculate_crc();
    void prepare_to_send();
    void print();
    void prepare_LR(uint8_t in_index);
    void prepare_LD(uint8_t reason);
    void prepare_LA(uint8_t seq_no);
};

class NewtToDockPipe : public Pipe {
    std::queue<Event> job_list_;

    MNPFrame *in_frame_ = nullptr;
    uint16_t in_frame_crsr_ = 0;
    enum class InState {  // TODO: move to class Frame?
        ABORT,                  // clear the in_buffer and start over
        WAIT_FOR_SYN,           // synchronous idle
        WAIT_FOR_DLE,           // data link escape
        WAIT_FOR_STX,           // start of text
        WAIT_FOR_HDR_SIZE,      // 0..254
        WAIT_FOR_HDR_TYPE,      // LR, LD, LT, or LA
        WAIT_FOR_HDR_DATA,      // header data
        WAIT_FOR_DATA,          // data link escape ends data
        WAIT_FOR_ETX,           // end of text
        WAIT_FOR_CRC_lo,        // low byte of CRC
        WAIT_FOR_CRC_hi,        // high byte of CRC
    } in_state_ = InState::WAIT_FOR_SYN;

    uint8_t in_seq_no_ = 0;

    MNPFrame *out_frame_ = nullptr;
    uint16_t out_frame_crsr_ = 0;

    void mnp_to_dock_state_machine();
    void start_next_job();
    void handle_newt_frame(MNPFrame *frame);

public:
    MNPFilter &filter_;
    NewtToDockPipe(MNPFilter &filter) : filter_(filter) { }
    void task();
    Result send(Event event) override;
    void add_job(Event event) { job_list_.push(event); }
};

class DockToNewtPipe : public Pipe {
    std::queue<Event> job_list_lt_;     // List all LT jobs here in a fifo.
    std::queue<Event> job_list_;        // Queue all other jobs here.

    // State machine for sending MNP frames to the Newton.
    MNPFrame *active_frame_ = nullptr;
    MNPFrame *ack_pending_frame_ = nullptr;
    enum class OutState {
        SEND_SYN,
        SEND_DLE,
        SEND_STX,
        SEND_HDR_SIZE,
        SEND_HDR_SIZE_DLE,
        SEND_HDR_DATA,
        SEND_HDR_DATA_DLE,
        SEND_DATA,
        SEND_DATA_DLE,
        SEND_DLE2,
        SEND_ETX,
        SEND_CRC_lo,
        SEND_CRC_hi,
    } out_state_ = OutState::SEND_SYN;
    uint16_t data_crsr_ = 0;
    uint8_t seq_no_ = 0; // sequence number for LT frames

    MNPFrame *in_frame = nullptr;


    void mnp_to_newt_state_machine();
    void release_frame(MNPFrame *frame);
    void retain_until_ack();
    void start_next_job();
    void flush_in_frame();
    void acknowledge_frame(uint8_t seq);

public:
    MNPFilter &filter_;
    DockToNewtPipe(MNPFilter &filter) : filter_(filter) { }
    void task();
    Result send(Event event) override;
    void add_job(Event event);
};

} // namespace nd

// ==== MNPFrame ===============================================================

/**
 * \brief Allocate a universal MNP frame.
 * All frames are held in a small pool inside MNPFilter.
 * \param filter The MNPFilter that owns this frame.
 */
MNPFrame::MNPFrame(MNPFilter &filter)
: filter_(filter)
{
	header.reserve(256);
	data.reserve(1024);     // TODO: maybe 256 is enough?
}

/**
 * \brief Clear the frame, but don't change `in_use`.
 */
void MNPFrame::clear() {
    // TODO: verify this!
	header.clear();
	data.clear();
	crc = 0;
	escaping_dle = false;
	crc_valid = false;
	// don't change `in_use`!
}

/**
 * \brief Get the type of the frame.
 */
uint8_t MNPFrame::type() {
    if (header.empty()) 
        return kMNP_Frame_Unknown;
    else
        return header[0];
}

/**
 * \brief Calculate the CRC16 checksum for this frame.
 * The CRC is calculated over the size, header, data, and ETX.
 * This does not store the CRC in the frame.
 */
uint16_t MNPFrame::calculate_crc() {
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
void MNPFrame::prepare_to_send() {
	crc = calculate_crc();
	crc_valid = true;
}

/**
 * \brief Print the contents of the frame.
 */
void MNPFrame::print() {
	Log.log("--- Frame ---\r\n");
	switch (type()) {
		case kMNP_Frame_LR: Log.log("Link Request\r\n"); break;
		case kMNP_Frame_LD: Log.log("Link Disconnect\r\n"); break;
		case kMNP_Frame_LT: Log.log("Link Transfer\r\n"); break;
		case kMNP_Frame_LA: Log.log("Link Acknowledge\r\n"); break;
		default: Log.logf("Unknown Frame Type %d\r\n", type()); break;
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

void MNPFrame::prepare_LR(uint8_t in_index) {
	clear();
	header = { 
		kMNP_Frame_LR, 
		      0x02, 0x01, 0x06, 0x01, 0x00, 0x00, 0x00,
		0x00, 0xFF, 0x02, 0x01, 0x02, 0x03, 0x01, 0x01, 
		0x04, 0x02, 0x40, 0x00, 0x08, 0x01, 0x03
	};
	prepare_to_send();
}

void MNPFrame::prepare_LD(uint8_t reason) {
	// 1 Protocol establishment phase error, LR expected but not received
	// 2 LR constant parameter 1 contains an unexpected value
	// 3 LR received with incompatible or unknown parameter value
	// 255 User-initiated disconnect
	clear();
	header.push_back(kMNP_Frame_LD);
	header.push_back(1);
	header.push_back(1);
	header.push_back(reason);
	prepare_to_send();
}

void MNPFrame::prepare_LA(uint8_t seq_no) {
	clear();
	header.push_back(kMNP_Frame_LA);
	header.push_back(seq_no);
	header.push_back(1); // TODO: credit
	prepare_to_send();
}

// ==== NewtToDockPipe =========================================================

/**
 * \brief Handle incoming events from the Newton endpoint.
 * This will process the MNP frames and send them to the Dock.
 */
void NewtToDockPipe::task() {
    if (out_frame_) {
        mnp_to_dock_state_machine();
    } else if (!job_list_.empty()) {
        start_next_job();
    }
}

void NewtToDockPipe::start_next_job() 
{
    if (out_frame_ || job_list_.empty()) return;

    Event event = job_list_.front();
    if (event.type() != Event::Type::MNP) {
        if (kLogMNPErrors) Log.log("NewtToDockPipe::start_next_job: dock job type not MNP\r\n");
        job_list_.pop();
    } else {
        switch (event.subtype()) {
            case Event::Subtype::MNP_DATA_TO_DOCK: // data() is index in in_pool
                out_frame_ = filter_.frame_pool_[event.data()];
                out_frame_crsr_ = 0;
                job_list_.pop();
                break;
            default:
                if (kLogMNPErrors) Log.log("NewtToDockPipe::start_next_job: unknown dock job\r\n");
                job_list_.pop();
                break;
        }
    }
}

void NewtToDockPipe::mnp_to_dock_state_machine() 
{
    MNPFrame *frame = out_frame_;
    if (!frame) return;
    Pipe *o = out();
    if (!o) {
        // TODO: release frame and remove it from the pipe.
        if (kLogMNPErrors) Log.log("DockToNewtPipe::mnp_to_dock_state_machine: No output pipe available.\r\n");
        return;
    }

    if (out_frame_crsr_ == 0) { // In order to have state 0, crsr is off by one!
        // Send the start frame marker.
        Result res = o->send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_START));
        if (res.ok()) {
            out_frame_crsr_++;
        }
    } else if (out_frame_crsr_<= frame->data.size()) {
        Result res = o->send(frame->data[out_frame_crsr_-1]);
        if (res.ok()) {
            out_frame_crsr_++;
        }
    } else {
        Result res = o->send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_END));
        if (res.ok()) {
            out_frame_crsr_ = 0; 
            out_frame_->in_use = false; // mark the frame as not in use anymore
            out_frame_ = nullptr; // release the frame
        }
    }
}

/** 
 * \brief Handle events comming from the Newton and hand them over to the MNP Filter.
 */
Result NewtToDockPipe::send(Event event) {
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
		return Result::OK__NOT_HANDLED;
	}

	// If we don't have an in_frame, try to get one from the pool.
	if (!in_frame_) {
		in_frame_ = filter_.acquire_frame();
		if (!in_frame_) {
			// No in frame is available. Tell the in pipe to resend the data again later.
			if (kLogMNPWarnings) Log.log("MNP Warning: `send` no frame available\r\n");
			return Result::REJECTED;
		}
	}

	// At this point, in_frame is guaranteed to point to a valid frame.
	uint8_t c = event.data();
	switch (in_state_) {
		case InState::ABORT:
			// -- abort the current frame and start over
			in_frame_->clear();
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
				in_frame_->clear();
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
			in_frame_->expected_header_size = c;
			in_state_ = InState::WAIT_FOR_HDR_TYPE;
			break;
        case InState::WAIT_FOR_HDR_TYPE:
			in_frame_->header.push_back(c);
			if (c == kMNP_Frame_LR) {
				if (kLogMNFlow) Log.log("(LR) ");
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else if (c == kMNP_Frame_LD) {
				if (kLogMNFlow) Log.log("(LD) ");
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else if (c == kMNP_Frame_LT) {
				if (kLogMNFlow) Log.log("(LT) ");
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else if (c == kMNP_Frame_LA) {
				if (kLogMNFlow) Log.log("(LA) ");
				in_state_ = InState::WAIT_FOR_HDR_DATA;
			} else {
				if (kLogMNPErrors) Log.logf("\r\nMNP ERROR: unsupported frame type %d.\r\n", c);
				in_state_ = InState::ABORT;
				break;
			}
			break;
		case InState::WAIT_FOR_HDR_DATA:
			if (c == 0x10) {
				if (in_frame_->escaping_dle) {
					in_frame_->escaping_dle = false;
					// fall through
				} else {
					in_frame_->escaping_dle = true;
					break;
				}
			}
			if (in_frame_->escaping_dle) {
				if (kLogMNPErrors) Log.logf("MNP ERROR: invalid DLE escape 0x10 0x%02x\r\n", c);
				in_state_ = InState::ABORT;
			}
			in_frame_->header.push_back(c);
			if (in_frame_->header.size() == in_frame_->expected_header_size) {
				in_state_ = InState::WAIT_FOR_DATA;
			}
			break;

		// -- read data until we reach the 0x10 0x03
		case InState::WAIT_FOR_DATA:
			if (c == 0x10) in_state_ = InState::WAIT_FOR_ETX;
			else in_frame_->data.push_back(c); // TODO: check for overflow
			break;

		// -- read the end of the block
		case InState::WAIT_FOR_ETX:
			if (c == 0x10) {
				in_frame_->data.push_back(c); // TODO: check for overflow
				in_state_ = InState::WAIT_FOR_DATA;
			} else if (c == 0x03) {
				in_state_ = InState::WAIT_FOR_CRC_lo;
			} else {
				in_state_ = InState::ABORT; // abort
			}
			break;

		// -- read the CRC	
		case InState::WAIT_FOR_CRC_lo:
			in_frame_->crc = c;
			in_state_ = InState::WAIT_FOR_CRC_hi;
			break;
		case InState::WAIT_FOR_CRC_hi:
			in_frame_->crc |= (c << 8);
			my_crc = in_frame_->calculate_crc();
			if (my_crc != in_frame_->crc) {
				if (kLogMNPErrors) Log.logf("MNP ERROR: CRC error 0x%04x != 0x%04x\r\n", in_frame_->crc, my_crc);
				in_state_ = InState::ABORT;
			} else {
				in_frame_->crc_valid = true;
				handle_newt_frame(in_frame_);
				in_state_ = InState::WAIT_FOR_SYN;
				in_frame_ = nullptr; // this method must acquire a new frame, handle_valid_in_frame must release the frame
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

/**
 * \brief Handle a valid incomming MNP frame.
 * This is called when the CRC of the incomming frame is valid.
 * We need to check the type of the frame and handle it accordingly.
 */
void NewtToDockPipe::handle_newt_frame(MNPFrame *frame) {
	// frame->print();
	switch (frame->type()) {
		case kMNP_Frame_LR: {
			// Handle Link Request
			// This can happen even if we think we are in connected state, so just renegotiate.
			// - reply with our LR frame to "negotiate"
			// - set the mnp state to connection pending
			if (filter_.mnp_state_ != MNPFilter::MNPState::DISCONNECTED) filter_.set_disconnected();
			filter_.set_negotiating();
			if (kLogMNPState) Log.log("\r\n**** MNP Negotiating\r\n");
			filter_.dock_->add_job(Event(Event::Type::MNP, Event::Subtype::MNP_SEND_LR, frame->pool_index));
            out()->send(Event(Event::Type::MNP, Event::Subtype::MNP_NEGOTIATING));
			// NOTE: the LR frame must be released by `dock` after creating the reply.
			break;
		}
		case kMNP_Frame_LD: {
			// Handle Link Disconnect
			// - do we need to acknowledge this (LA)?
			// - set the mnp state to disconnected
			if (kLogMNPState) Log.log("\r\n**** MNP Disconnected\r\n");
			filter_.release_frame(frame);
            // We *could* acknowledge this, maybe later
            out()->send(Event(Event::Type::MNP, Event::Subtype::MNP_DISCONNECTED));
			filter_.set_disconnected();
			// TODO: we can print the reason for disconnecting
			break;
		}
		case kMNP_Frame_LT: {
			// Handle Link Transfer
			// Input Pipe sent us data. Check if the sequence number is correct.
			// - if so, send an LA frame to acknowledge the data and forward it to the Dock
			// - if not, send an LA frame with the last sequence number we received
			if (frame->header.size() == 2) {
				// TODO: this assumes that the previous frame was received correctly.
				// We should only set the new seq# if it is the old seq# plus 1.
				in_seq_no_ = frame->header[1];
			} else if (frame->header.size() == 4) {
				// TODO: this assumes that the previous frame was received correctly.
				// We should only set the new seq# if it is the old seq# plus 1.
				in_seq_no_ = frame->header[3];
			} else {
				if (kLogMNPErrors) Log.logf("MNP ERROR: Unexpected LT header size %d\r\n", frame->header.size());
				filter_.release_frame(frame);
				break; // we ignore the frame because we can't get the sequence number
			}
			// TODO: we must verify that this is a valid sequence number
			if (filter_.mnp_state_ == MNPFilter::MNPState::CONNECTED) {
				if (kLogMNPState) Log.log("\r\n**** MNP Data Packet\r\n");
				filter_.dock_->add_job(Event(Event::Type::MNP, Event::Subtype::MNP_SEND_LA, in_seq_no_));
				filter_.newt_->add_job(Event(Event::Type::MNP, Event::Subtype::MNP_DATA_TO_DOCK, frame->pool_index));
				// NOTE: the LT handler must eventually release the in_frame
			} else {
				if (kLogMNPErrors) Log.log("\r\n**** ERROR: MNP Data Packet while not connected\r\n");
				filter_.dock_->add_job(Event(Event::Type::MNP, Event::Subtype::MNP_SEND_LD, 255));
				filter_.release_frame(frame);
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
			if (frame->header.size() == 3) {
				seq = frame->header[1];
				credit = frame->header[2];
			} else if (frame->header.size() == 7) {
				seq = frame->header[3];
				credit = frame->header[6];
			} else {
				if (kLogMNPErrors) Log.logf("MNP ERROR: Unexpected LA header size %d\r\n", frame->header.size());
			}
			if (filter_.mnp_state_ == MNPFilter::MNPState::CONNECTED) {
				// TODO: is there a Frame that we need to resend?
                filter_.dock_->add_job(Event(Event::Type::MNP, Event::Subtype::MNP_RECEIVED_LA, seq));
				// TODO: also, is there a timeout with no LA?
			} else if (filter_.mnp_state_ == MNPFilter::MNPState::NEGOTIATING) {
				// TODO: add a timeout to an earlier LR with no LA!
				filter_.set_connected();
                filter_.dock_->add_job(Event(Event::Type::MNP, Event::Subtype::MNP_RECEIVED_LA, seq + 0x0100));
				if (kLogMNPState) Log.log("\r\n**** MNP Connected\r\n");
                out()->send(Event(Event::Type::MNP, Event::Subtype::MNP_CONNECTED));
			} else {
			}
			filter_.release_frame(frame);
			break;
		}
		default:
			filter_.release_frame(frame);
			if (kLogMNPErrors) Log.log("MNP ERROR: handle_valid_in_frame: Unknown frame type %d\r\n", frame->type());
			break;
	}
}

// ==== DockToNewtPipe =========================================================

/**
 * \brief Handle incoming events from the Dock endpoint.
 * This will create MNP frames from the data stream and send them to the Newton.
 */
void DockToNewtPipe::task() 
{
    if (active_frame_) {
        mnp_to_newt_state_machine();        
    } else if (!job_list_.empty() || !job_list_lt_.empty()) {
        start_next_job();
    }
}

/**
 * \brief Send out a frame toward the Newton in a state machine.
 */
void DockToNewtPipe::mnp_to_newt_state_machine() 
{
    MNPFrame *frame = active_frame_;
    if (!frame) return;

	Pipe *o = out();
    if (!o) {
        // TODO: release frame and remove it from the pipe.
        if (kLogMNPErrors) Log.log("DockToNewtPipe::mnp_to_newt_state_machine: No output pipe available.\r\n");
        return;
    }

    uint8_t c = 0;
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

		case OutState::SEND_HDR_SIZE_DLE:
		case OutState::SEND_HDR_SIZE:
            c = frame->header.size();
			if (o->send(c).ok()) {
				data_crsr_ = 0;
                if ((c == 0x10) && (out_state_== OutState::SEND_HDR_SIZE)) {
                    out_state_ = OutState::SEND_HDR_SIZE_DLE;
                } else {
                    out_state_ = OutState::SEND_HDR_DATA;
                }
			}
			break;
		case OutState::SEND_HDR_DATA_DLE:
		case OutState::SEND_HDR_DATA:
            c = frame->header[data_crsr_];
			if (o->send(c).ok()) {
				if ((c == 0x010) && (out_state_ == OutState::SEND_HDR_DATA)) {
					out_state_ = OutState::SEND_HDR_DATA_DLE;
				} else {
                    data_crsr_++;
                    if (data_crsr_ == frame->header.size()) {
                        data_crsr_ = 0;
						if (frame->data.size() == 0) {
							out_state_ = OutState::SEND_DLE2; // no data, send DLE2
						} else {
							out_state_ = OutState::SEND_DATA; // data follows
						}
                    } else {
						// still more header data to send (overwrite SEND_HDR_DATA_DLE)
						out_state_ = OutState::SEND_HDR_DATA;
					}
                }
			}
			break;
		case OutState::SEND_DATA_DLE:
		case OutState::SEND_DATA:
            c = frame->data[data_crsr_];
			if (o->send(c).ok()) {
                if ((c == 0x10) && (out_state_ == OutState::SEND_DATA)) {
					out_state_ = OutState::SEND_DATA_DLE;
				} else {
					data_crsr_++;
                    if (data_crsr_ == frame->data.size()) {
                        data_crsr_ = 0;
                        out_state_ = OutState::SEND_DLE2;
                    } else {
						// still more data to send (overwrite SEND_DATA_DLE)
						out_state_ = OutState::SEND_DATA;
					}
				}
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
                if (frame->header[0] == kMNP_Frame_LT) {
                    // If this is a Link Transfer frame, we must wait for an ACK.
                    retain_until_ack();
                } else {
                    // For all other frames, we can release the frame.
                    release_frame(frame);
                }
			}
			break;
	}
}

/**
 * \brief Release the currently active frame and reset the state machine.
 */
void DockToNewtPipe::release_frame(MNPFrame *frame) {
    if (!frame) {
        if (kLogMNPErrors) Log.log("DockToNewtPipe::release_frame: Error: no frame to release.\r\n");
        return;
    }
    if (frame == active_frame_) {
        active_frame_->clear();             // clear the frame data
        active_frame_->in_use = false;      // mark the frame as not in use
        active_frame_ = nullptr;            // no active frame anymore
        out_state_ = OutState::SEND_SYN;    // reset the state machine
    } else if (frame == ack_pending_frame_) {
        ack_pending_frame_->clear();         // clear the frame data
        ack_pending_frame_->in_use = false;  // mark the frame as not in use
        ack_pending_frame_ = nullptr;        // no pending frame anymore
    } else {
        if (kLogMNPErrors) Log.log("DockToNewtPipe::release_frame: Error: frame not active or pending.\r\n");
    }
}

/**
 * \brief Retain the current frame until an LA is received.
 * This will keep the current frame in use and wait for an LA from the Newton.
 * If a wrong LA sequence number is received,
 * \todo or the time until LA is too long,
 * the frame will be resent up to three times.
 */
void DockToNewtPipe::retain_until_ack() 
{
    if (ack_pending_frame_) {
        if (kLogMNPErrors) Log.log("DockToNewtPipe::retain_until_ack: Error: already waiting for ACK.\r\n");
        return;
    }
    ack_pending_frame_ = active_frame_;
    active_frame_ = nullptr; // clear the active frame
    out_state_ = OutState::SEND_SYN; // reset the state machine
}

/**
 * \brief Start the next job from the job list.
 * This will take the next job from the job list and prepare it for sending.
 * If there is no job, it will do nothing.
 */
void DockToNewtPipe::start_next_job() 
{
    if (!active_frame_ && !job_list_.empty()) 
    {
        Event event = job_list_.front();
        if (event.type() != Event::Type::MNP) {
            if (kLogMNPErrors) Log.log("kToNewtPipe::start_next_job: Error: job type not MNP\r\n");
            job_list_.pop();
            return;
        }

        MNPFrame *my_frame = nullptr;
        switch (event.subtype()) {
            case Event::Subtype::MNP_SEND_LA: // data() is in sequence number
                my_frame = filter_.acquire_frame();
                if (!my_frame) {
                    if (kLogMNPWarnings) Log.log("MNP_SEND_LA: no out frame available, try again.\r\n");
                } else {
                    my_frame->prepare_LA(event.data());
                    out_state_ = OutState::SEND_SYN;
                    active_frame_ = my_frame; // TODO: trigger sending state
                    job_list_.pop();
                }
                break;
            case Event::Subtype::MNP_SEND_LD: // data() is disconnect reason
                my_frame = filter_.acquire_frame();
                if (!my_frame) {
                    if (kLogMNPWarnings) Log.log("MNP_SEND_LD: no out frame available, try again.\r\n");
                } else {
                    my_frame->prepare_LD(event.data());
                    out_state_ = OutState::SEND_SYN;
                    active_frame_ = my_frame; // TODO: trigger sending state
                    job_list_.pop();
                }
                break;
            case Event::Subtype::MNP_SEND_LR: // data() is original LR index on in_pool
                my_frame = filter_.acquire_frame();
                if (!my_frame) {
                    if (kLogMNPWarnings) Log.log("MNP_SEND_LR: no out frame available, try again.\r\n");
                } else {
                    my_frame->prepare_LR(event.data());
                    out_state_ = OutState::SEND_SYN;
                    active_frame_ = my_frame; // TODO: trigger sending state
                    job_list_.pop();
                    MNPFrame *in_frame = filter_.frame_pool_[event.data()];
                    filter_.release_frame(in_frame);
                }
                break;
			case Event::Subtype::MNP_RECEIVED_LA: // data() is sequence number
				if (event.data() > 0xff) {
					// This is a special case, we received a LA for the LR frame.
					seq_no_ = event.data() & 0xff; // store the sequence number
				} else if (ack_pending_frame_) {
					// We are waiting for an LA for the last LT frame.
					if (ack_pending_frame_->header[1] == event.data()) {
						// We got the correct LA, release the frame.
						if (kLogDock) Log.logf("DockToNewtPipe::start_next_job: Acknowledging frame %d\r\n", event.data());
						release_frame(ack_pending_frame_);
					} else {
						// Wrong sequence number, we need to resend the frame.
						if (kLogDock) Log.logf("DockToNewtPipe::start_next_job: Wrong sequence number %d, resending frame.\r\n", event.data());
						add_job(Event(Event::Type::MNP, Event::Subtype::MNP_SEND_LT, ack_pending_frame_->pool_index));
						ack_pending_frame_ = nullptr; // clear the pending frame
					}
				} else {
					if (kLogMNPErrors) Log.log("DockToNewtPipe::start_next_job: No frame to acknowledge.\r\n");
				}
				job_list_.pop();
				break;
            default:
                if (kLogMNPErrors) Log.log("kToNewtPipe::start_next_job: Error: unknown job subtype\r\n");
                job_list_.pop();
                break;
        }
    }
    if (!active_frame_ && !ack_pending_frame_ && !job_list_lt_.empty()) 
    {
        Event event = job_list_lt_.front();
        if (event.type() != Event::Type::MNP) {
            if (kLogMNPErrors) Log.log("kToNewtPipe::start_next_job: Error: job type not MNP\r\n");
            job_list_lt_.pop();
            return;
        }

        switch (event.subtype()) {
            case Event::Subtype::MNP_SEND_LT: // data() is index in frame_pool
                active_frame_ = filter_.frame_pool_[event.data()];
                active_frame_->in_use = true; // mark the frame as in use
                data_crsr_ = 0; // reset the data cursor
                out_state_ = OutState::SEND_SYN; // reset the state machine
                job_list_lt_.pop();
                break;
            default:
                if (kLogMNPErrors) Log.log("kToNewtPipe::start_next_job: Error: unknown job subtype\r\n");
                job_list_lt_.pop();
                break;
        }
    
    }
}   

/**
 * \brief Handle events comming from the Dock and hand them over to the MNP Filter as LT events.
 */
Result DockToNewtPipe::send(Event event) 
{
    if (event.type() == Event::Type::MNP) {
        if ((event.subtype() == Event::Subtype::MNP_FRAME_START) || (event.subtype() == Event::Subtype::MNP_FRAME_END)){
            if (in_frame) {
                // if (kLogMNPErrors) Log.log("MNPFilter::dock_send: flush dock_in_frame on new frame\r\n");
                flush_in_frame();
            }
            return Result::OK;
        }
    } else if (event.type() == Event::Type::DATA) {
        if (!in_frame) {
            if (ack_pending_frame_ || !job_list_lt_.empty() || (active_frame_ && (active_frame_->header[0] == kMNP_Frame_LT))) {
                // We are waiting for an LA for the last LT frame, or there are alrady other LT jobs in the queue.
                //if (kLogMNPErrors) Log.log("DockToNewtPipe::send: Error: waiting for LA, cannot send new event.\r\n");
                return Result::REJECTED;
            }
            in_frame = filter_.acquire_frame();
            if (!in_frame) {
                if (kLogMNPErrors) Log.log("DockToNewtPipe::send: Error: no frame available.\r\n");
                return Result::REJECTED;
            }
        }
        if (kLogDock) Log.logf("+%02x ", event.data());
        in_frame->data.push_back(event.data());
        if (in_frame->data.size() >= MNPFilter::kMaxData) {
            // We have enough data to send a LT frame.
            if (kLogDock) Log.log("\r\nDock: LT frame ready\r\n");
            flush_in_frame();
        }
        return Result::OK;
	}
	if (kLogMNPErrors) Log.logf("MNPFilter::dock_send: unknown event type (%d:%d:%d)\r\n", (int)event.type(), (int)event.subtype(), (int)event.data());
	return Result::OK__NOT_HANDLED;
}

void DockToNewtPipe::add_job(Event event) { 
    if ((event.type() == Event::Type::MNP) && (event.subtype() == Event::Subtype::MNP_SEND_LT)) {
        job_list_lt_.push(event);
    } else {
        job_list_.push(event);
    }
}

void DockToNewtPipe::flush_in_frame() {
    MNPFrame *f = in_frame;
    in_frame = nullptr;
    f->header.clear();
    f->header.push_back(kMNP_Frame_LT);
    f->header.push_back(++seq_no_);
    f->prepare_to_send();
    add_job(Event(Event::Type::MNP, Event::Subtype::MNP_SEND_LT, f->pool_index));
}

void DockToNewtPipe::acknowledge_frame(uint8_t seq) {
    if (ack_pending_frame_) {
        if (ack_pending_frame_->header[1] == seq) {
            // We got the correct LA, release the frame.
            if (kLogDock) Log.logf("DockToNewtPipe::acknowledge_frame: Acknowledging frame %d\r\n", seq);
            release_frame(ack_pending_frame_);
        } else {
            // Wrong sequence number, we need to resend the frame.
            if (kLogDock) Log.logf("DockToNewtPipe::acknowledge_frame: Wrong sequence number %d, resending frame.\r\n", seq);
            add_job(Event(Event::Type::MNP, Event::Subtype::MNP_SEND_LT, ack_pending_frame_->pool_index));
            ack_pending_frame_ = nullptr; // clear the pending frame
        }
    } else {
        if (kLogMNPErrors) Log.log("DockToNewtPipe::acknowledge_frame: No frame to acknowledge.\r\n");
    }
}

// ==== MNPFilter ==============================================================

/**
 * \brief Create a new MNPFilter object.
 * \todo This will be renamed to MNPFilter or similar
 * \todo This should not be an Endpoint, but a filter that forwards the decoded
 *    MNP data as a stream down the pipe to a Dock class.
 */
MNPFilter::MNPFilter(Scheduler &scheduler) 
:   super(scheduler),
	newt_(new NewtToDockPipe(*this)),
	dock_(new DockToNewtPipe(*this)),
	newt(*newt_),
	dock(*dock_)
{
    for (auto &f : frame_pool_) {
        f = new MNPFrame(*this);
    }
}

/**
 * \brief Release all resources.
 * This will delete the dock and newt pipes, and all frames in the pool.
 */
MNPFilter::~MNPFilter() 
{
    delete dock_;
    delete newt_;
    for (auto &f : frame_pool_) {
        delete f;
    }
}

/**
 * \brief Handle out pipes and timeouts.
 */
Result MNPFilter::task() {
    dock_->task();
    newt_->task();
    return Result::OK;
}

/**
 * \brief Not used.
 */
Result MNPFilter::send(Event event) {
    Log.log("ERROR: MNPFilter::send() not implemented. Send to `newt` or `dock` pipe instead!\r\n");
    return Result::OK__NOT_CONNECTED;
}

/**
 * \brief Acquire a free MNP frame from the pool.
 * \return Pointer to the acquired frame, or nullptr if no frame is available.
 */
MNPFrame *MNPFilter::acquire_frame() {
	for (auto &frame : frame_pool_) {
		if (!frame->in_use) {
			frame->clear(); // clear the frame data
			frame->in_use = true;
			return frame;
		}
	}
	return nullptr;
}

void MNPFilter::release_frame(MNPFrame *frame) {
    if (frame) {
        frame->clear();
        frame->in_use = false;
    }
}


void MNPFilter::set_disconnected() {
	// TODO: free up all in and out frames
	// TODO: clear the in and out jobs queue
	mnp_state_ = MNPState::DISCONNECTED;
    // TODO: call disconnected onnewt and dock pipes
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


// ==== CRC16 ==================================================================

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
