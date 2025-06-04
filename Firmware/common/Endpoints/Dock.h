//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_ENDPOINTS_DOCK_H
#define ND_ENDPOINTS_DOCK_H

#include "common/Endpoint.h"

#include <queue>
#include <vector>

#ifndef ND_FOURCC
#  if __BYTE_ORDER == __LITTLE_ENDIAN 
#    define ND_FOURCC(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#  elif __BYTE_ORDER == __BIG_ENDIAN
#    define ND_FOURCC(a, b, c, d) ((uint32_t)(d) | ((uint32_t)(c) << 8) | ((uint32_t)(b) << 16) | ((uint32_t)(a) << 24))
#  else
#    error "Can't determine endianess"
#  endif
#endif

namespace nd {

class Dock : public Endpoint 
{
    typedef Endpoint super;

    // See DockProtocol.h
    constexpr static uint32_t kDRequestToDock = ND_FOURCC('r', 't', 'd', 'k'); // Newt -> Dock
    constexpr static uint32_t kDInitiateDocking = ND_FOURCC('d', 'o', 'c', 'k'); // Dock -> Newt
    constexpr static uint32_t kSettingUpSession = 1;
    constexpr static uint32_t kLoadPackageSession = 4;
    void send_cmd_dock(uint32_t session_type);
    constexpr static uint32_t kDNewtonName = ND_FOURCC('n', 'a', 'm', 'e'); // Newt -> Dock
    constexpr static uint32_t kDDesktopInfo = ND_FOURCC('d', 'i', 'n', 'f'); // Dock -> Newt
    void send_cmd_dinf();
    constexpr static uint32_t kDNewtonInfo = ND_FOURCC('n', 'i', 'n', 'f'); // Newt -> Dock
    constexpr static uint32_t kDWhichIcons = ND_FOURCC('w', 'i', 'c', 'n'); // Dock -> Newt
    void send_cmd_wicn(uint32_t icon_map);
    constexpr static uint32_t kInstallIcon = 1 << 2;
    constexpr static uint32_t kDResult = ND_FOURCC('d', 'r', 'e', 's'); // Dock <-> Newt
    void send_cmd_dres(uint32_t error_code);
    constexpr static uint32_t kDSetTimeout = ND_FOURCC('s', 't', 'i', 'm'); // Dock -> Newt
    void send_cmd_stim();
    constexpr static uint32_t kDPassword = ND_FOURCC('p', 'a', 's', 's'); // Dock <-> Newt
    void send_cmd_pass();
    constexpr static uint32_t kDHello = ND_FOURCC('h', 'e', 'l', 'o'); // Dock <-> Newt
    void send_cmd_helo();

    constexpr static uint32_t kDRequestToBrowse = ND_FOURCC('r', 't', 'b', 'r'); // Newt ->Dock
    constexpr static uint32_t kDGetDefaultPath = ND_FOURCC('d', 'p', 't', 'h'); // Newt -> Dock
    void send_cmd_path();
    constexpr static uint32_t kDPath = ND_FOURCC('p', 'a', 't', 'h'); // Dock -> Newt
    constexpr static uint32_t kDGetFilesAndFolders = ND_FOURCC('g', 'f', 'i', 'l'); // Newt -> Dock
    void send_cmd_file();
    constexpr static uint32_t kDFilesAndFolders = ND_FOURCC('f', 'i', 'l', 'e'); // Dock -> Newt

    constexpr static uint32_t kDLoadPackageFile = ND_FOURCC('l', 'p', 'f', 'l'); // Newt -> Dock

    struct Data {
        const std::vector<uint8_t> *bytes_;
        uint32_t pos_ = 0; // current position in the data
        bool start_frame_ = false; // if true, send a start frame marker
        bool end_frame_ = false; // if true, send an end frame marker
        bool free_after_send_ = false; // if true, the data will be freed after sending
    };
    std::queue<Data> data_queue_; // queue of data to be sent

    std::vector<uint8_t> in_data_;
    union {
        uint8_t cmd_[5] = {0}; // buffer for the command
        uint32_t cmd;
    };
    uint32_t size = 0;
    uint32_t aligned_size = 0;
    uint8_t in_stream_state_ = 0;
    uint32_t in_index_ = 0;

    uint32_t dres_next_ = 0;

    // Various flags to store the current state of the docking session
    bool connected_ = false; // true if we are connected to the Dock
    uint32_t hello_timer_ = 0;

    uint32_t newt_challenge_hi = 0;
    uint32_t newt_challenge_lo = 0;

    bool package_sent = false;

public:
    Dock(Scheduler &scheduler) : Endpoint(scheduler) { 
        in_data_.reserve(400); // reserve space for incoming data
    }
    ~Dock() override = default;
    Dock(const Dock&) = delete;
    Dock& operator=(const Dock&) = delete;
    Dock(Dock&&) = delete;
    Dock& operator=(Dock&&) = delete;

    // -- Pipe
    Result send(Event event) override;
    // Result rush(Event event) override;
    // Result rush_back(Event event) override;

    // -- Task
    // Result init() override;
    Result task() override;
    // Result signal(Event event) override;

    // -- Dock specific methods
    void process_command();
    void send_lpkg();
    void send_disc();


private:
    typedef struct
    {
        uint32_t	fNewtonID;				/* A unique id to identify a particular newton */
        uint32_t	fManufacturer;			/* A decimal integer indicating the manufacturer of the device */
        uint32_t	fMachineType;			/* A decimal integer indicating the hardware type of the device */
        uint32_t	fROMVersion;			/* A decimal number indicating the major and minor ROM version numbers */
                                                /* The major number is in front of the decimal, the minor number after */
        uint32_t	fROMStage;				/* A decimal integer indicating the language (English, German, French) */
                                                /* and the stage of the ROM (alpha, beta, final) */
        uint32_t	fRAMSize;
        uint32_t	fScreenHeight;			/* An integer representing the height of the screen in pixels */
        uint32_t	fScreenWidth;			/* An integer representing the width of the screen in pixels */
        uint32_t	fPatchVersion;			/* 0 on an unpatched Newton and nonzero on a patched Newton */
        uint32_t	fNOSVersion;
        uint32_t	fInternalStoreSig;	/* signature of the internal store */
        uint32_t	fScreenResolutionV;	/* An integer representing the number of vertical pixels per inch */
        uint32_t	fScreenResolutionH;	/* An integer representing the number of horizontal pixels per inch */
        uint32_t	fScreenDepth;			/* The bit depth of the LCD screen */
        uint32_t	fSystemFlags;			/* various bit flags */
                                                /* 1 = has serial number */
                                                /* 2 = has target protocol */
        uint32_t	fSerialNumber[2];
        uint32_t	fTargetProtocol;
    } NewtonInfo;

};

#if 0

#define ERRBASE_DOCKER			(-28000)	// Docker errors

#define kDockErrBadStoreSignature				(ERRBASE_DOCKER -  1)
#define kDockErrBadEntry							(ERRBASE_DOCKER -  2)
#define kDockErrAborted								(ERRBASE_DOCKER -  3)
#define kDockErrBadQuery							(ERRBASE_DOCKER -  4)
#define kDockErrReadEntryError					(ERRBASE_DOCKER -  5)
#define kDockErrBadCurrentSoup					(ERRBASE_DOCKER -  6)
#define kDockErrBadCommandLength					(ERRBASE_DOCKER -  7)
#define kDockErrEntryNotFound						(ERRBASE_DOCKER -  8)
#define kDockErrBadConnection						(ERRBASE_DOCKER -  9)
#define kDockErrFileNotFound						(ERRBASE_DOCKER - 10)
#define kDockErrIncompatibleProtocol			(ERRBASE_DOCKER - 11)
#define kDockErrProtocolError						(ERRBASE_DOCKER - 12)
#define kDockErrDockingCanceled					(ERRBASE_DOCKER - 13)
#define kDockErrStoreNotFound						(ERRBASE_DOCKER - 14)
#define kDockErrSoupNotFound						(ERRBASE_DOCKER - 15)
#define kDockErrBadHeader							(ERRBASE_DOCKER - 16)
#define kDockErrOutOfMemory						(ERRBASE_DOCKER - 17)
#define kDockErrNewtonVersionTooNew				(ERRBASE_DOCKER - 18)
#define kDockErrPackageCantLoad					(ERRBASE_DOCKER - 19)
#define kDockErrProtocolExtAlreadyRegistered	(ERRBASE_DOCKER - 20)
#define kDockErrRemoteImportError 				(ERRBASE_DOCKER - 21)
#define kDockErrBadPasswordError					(ERRBASE_DOCKER - 22)
#define kDockErrRetryPW								(ERRBASE_DOCKER - 23)
#define kDockErrIdleTooLong						(ERRBASE_DOCKER - 24)
#define kDockErrOutOfPower							(ERRBASE_DOCKER - 25)
#define kDockErrBadCursor							(ERRBASE_DOCKER - 26)
#define kDockErrAlreadyBusy						(ERRBASE_DOCKER - 27)
#define kDockErrDesktopError						(ERRBASE_DOCKER - 28)
#define kDockErrCantConnectToModem				(ERRBASE_DOCKER - 29)
#define kDockErrDisconnected						(ERRBASE_DOCKER - 30)
#define kDockErrAccessDenied						(ERRBASE_DOCKER - 31)

#define ERRBASE_DOCKER_			(ERRBASE_DOCKER - 100)	// More Docker errors

#define kDockErrDisconnectDuringRead			(ERRBASE_DOCKER_)
#define kDockErrReadFailed							(ERRBASE_DOCKER_ -  1)
#define kDockErrCommunicationsToolNotFound	(ERRBASE_DOCKER_ -  2)
#define kDockErrInvalidModemToolVersion		(ERRBASE_DOCKER_ -  3)
#define kDockErrCardNotInstalled					(ERRBASE_DOCKER_ -  4)
#define kDockErrBrowserFileNotFound				(ERRBASE_DOCKER_ -  5)
#define kDockErrBrowserVolumeNotFound			(ERRBASE_DOCKER_ -  6)
#define kDockErrBrowserPathNotFound				(ERRBASE_DOCKER_ -  7)
#endif


} // namespace nd

#endif // ND_ENDPOINTS_DOCK_H