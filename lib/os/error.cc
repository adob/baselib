#include <cerrno>
#include <string.h>
#include <errno.h>

#include "error.h"
#include "../fs.h"
using namespace lib;

namespace lib::os {

    void Errno::describe(io::OStream &out) const {
        char buf[1024];

        char *s = strerror_r(this->code, buf, sizeof(buf));
        out.write(str::from_c_str(s), error::ignore);
    }

    bool Errno::is(TypeID target) const {
        if (target == type_id<fs::ErrPermission>) {
            return code ==  EACCES || code == EPERM;
        }

        if (target == type_id<fs::ErrExist>) {
            return code == EEXIST || code == ENOTEMPTY;
        }

        if (target == type_id<fs::ErrNotExist>) {
            return code == ENOENT;
        }

        if (target == type_id<errors::ErrUnsupported>) {
            return code == ENOSYS || code == ENOTSUP || code == EOPNOTSUPP;
        }

        return false;
    }
//     Error ErrPERM("Operation not permitted");
//     Error ErrNOENT("No such file or directory");
//     Error ErrSRCH("No such process");
//     Error ErrINTR("Interrupted system call");
//     Error ErrIO("I/O error");
//     Error ErrNXIO("No such device or address");
//     Error Err2BIG("Argument list too long");
//     Error ErrNOEXEC("Exec format error");
//     Error ErrBADF("Bad file number");
//     Error ErrCHILD("No child processes");
//     Error ErrAGAIN("Try again");
//     Error ErrNOMEM("Out of memory");
//     Error ErrACCES("Permission denied");
//     Error ErrFAULT("Bad address");
//     Error ErrNOTBLK("Block device required");
//     Error ErrBUSY("Device or resource busy");
//     Error ErrEXIST("File exists");
//     Error ErrXDEV("Cross-device link");
//     Error ErrNODEV("No such device");
//     Error ErrNOTDIR("Not a directory");
//     Error ErrISDIR("Is a directory");
//     Error ErrINVAL("Invalid argument");
//     Error ErrNFILE("File table overflow");
//     Error ErrMFILE("Too many open files");
//     Error ErrNOTTY("Not a typewriter");
//     Error ErrTXTBSY("Text file busy");
//     Error ErrFBIG("File too large");
//     Error ErrNOSPC("No space left on device");
//     Error ErrSPIPE("Illegal seek");
//     Error ErrROFS("Read-only file system");
//     Error ErrMLINK("Too many links");
//     Error ErrPIPE("Broken pipe");
//     Error ErrDOM("Math argument out of domain of func");
//     Error ErrRANGE("Math result not representable");

//     Error ErrDEADLK("Resource deadlock would occur");
//     Error ErrNAMETOOLONG("File name too long");
//     Error ErrNOLCK("No record locks available");

//     Error ErrNOSYS("Invalid system call number");

//     Error ErrNOTEMPTY("Directory not empty");
//     Error ErrLOOP("Too many symbolic links encountered");
//     // deferror ErrWOULDBLOCK("Operation would block");
//     Error ErrNOMSG("No message of desired type");
//     Error ErrIDRM("Identifier removed");
//     Error ErrCHRNG("Channel number out of range");
//     Error ErrL2NSYNC("Level not synchronized");
//     Error ErrL3HLT("Level halted");
//     Error ErrL3RST("Level reset");
//     Error ErrLNRNG("Link number out of range");
//     Error ErrUNATCH("Protocol driver not attached");
//     Error ErrNOCSI("No CSI structure available");
//     Error ErrL2HLT("Level halted");
//     Error ErrBADE("Invalid exchange");
//     Error ErrBADR("Invalid request descriptor");
//     Error ErrXFULL("Exchange full");
//     Error ErrNOANO("No anode");
//     Error ErrBADRQC("Invalid request code");
//     Error ErrBADSLT("Invalid slot");

//     // deferror ErrDEADLOCK("Resource deadlock would occur");

//     Error ErrBFONT("Bad font file format");
//     Error ErrNOSTR("Device not a stream");
//     Error ErrNODATA("No data available");
//     Error ErrTIME("Timer expired");
//     Error ErrNOSR("Out of streams resources");
//     Error ErrNONET("Machine is not on the network");
//     Error ErrNOPKG("Package not installed");
//     Error ErrREMOTE("Object is remote");
//     Error ErrNOLINK("Link has been severed");
//     Error ErrADV("Advertise error");
//     Error ErrSRMNT("Srmount error");
//     Error ErrCOMM("Communication error on send");
//     Error ErrPROTO("Protocol error");
//     Error ErrMULTIHOP("Multihop attempted");
//     Error ErrDOTDOT("RFS specific error");
//     Error ErrBADMSG("Not a data message");
//     Error ErrOVERFLOW("Value too large for defined data type");
//     Error ErrNOTUNIQ("Name not unique on network");
//     Error ErrBADFD("File descriptor in bad state");
//     Error ErrREMCHG("Remote address changed");
//     Error ErrLIBACC("Can not access a needed shared library");
//     Error ErrLIBBAD("Accessing a corrupted shared library");
//     Error ErrLIBSCN(".lib section in a.out corrupted");
//     Error ErrLIBMAX("Attempting to link in too many shared libraries");
//     Error ErrLIBEXEC("Cannot exec a shared library directly");
//     Error ErrILSEQ("Illegal byte sequence");
//     Error ErrRESTART("Interrupted system call should be restarted");
//     Error ErrSTRPIPE("Streams pipe error");
//     Error ErrUSERS("Too many users");
//     Error ErrNOTSOCK("Socket operation on non-socket");
//     Error ErrDESTADDRREQ("Destination address required");
//     Error ErrMSGSIZE("Message too long");
//     Error ErrPROTOTYPE("Protocol wrong type for socket");
//     Error ErrNOPROTOOPT("Protocol not available");
//     Error ErrPROTONOSUPPORT("Protocol not supported");
//     Error ErrSOCKTNOSUPPORT("Socket type not supported");
//     Error ErrOPNOTSUPP("Operation not supported on transport endpoint");
//     Error ErrPFNOSUPPORT("Protocol family not supported");
//     Error ErrAFNOSUPPORT("Address family not supported by protocol");
//     Error ErrADDRINUSE("Address already in use");
//     Error ErrADDRNOTAVAIL("Cannot assign requested address");
//     Error ErrNETDOWN("Network is down");
//     Error ErrNETUNREACH("Network is unreachable");
//     Error ErrNETRESET("Network dropped connection because of reset");
//     Error ErrCONNABORTED("Software caused connection abort");
//     Error ErrCONNRESET("Connection reset by peer");
//     Error ErrNOBUFS("No buffer space available");
//     Error ErrISCONN("Transport endpoint is already connected");
//     Error ErrNOTCONN("Transport endpoint is not connected");
//     Error ErrSHUTDOWN("Cannot send after transport endpoint shutdown");
//     Error ErrTOOMANYREFS("Too many references: cannot splice");
//     Error ErrTIMEDOUT("Connection timed out");
//     Error ErrCONNREFUSED("Connection refused");
//     Error ErrHOSTDOWN("Host is down");
//     Error ErrHOSTUNREACH("No route to host");
//     Error ErrALREADY("Operation already in progress");
//     Error ErrINPROGRESS("Operation now in progress");
//     Error ErrSTALE("Stale file handle");
//     Error ErrUCLEAN("Structure needs cleaning");
//     Error ErrNOTNAM("Not a XENIX named type file");
//     Error ErrNAVAIL("No XENIX semaphores available");
//     Error ErrISNAM("Is a named type file");
//     Error ErrREMOTEIO("Remote I/O error");
//     Error ErrDQUOT("Quota exceeded");

//     Error ErrNOMEDIUM("No medium found");
//     Error ErrMEDIUMTYPE("Wrong medium type");
//     Error ErrCANCELED("Operation Canceled");
//     Error ErrNOKEY("Required key not available");
//     Error ErrKEYEXPIRED("Key has expired");
//     Error ErrKEYREVOKED("Key has been revoked");
//     Error ErrKEYREJECTED("Key was rejected by service");

//     Error ErrOWNERDEAD("Owner died");
//     Error ErrNOTRECOVERABLE("State not recoverable");

//     Error ErrRFKILL("Operation not possible due to RF-kill");

//     Error ErrHWPOISON("Memory page has hardware error");
    
//     Error ErrUnknown("Unknown error");
// }

// Error &os::from_errno(int n) {
//     switch (n) {
//         case EPERM: return ErrPERM;    
//         case ENOENT: return ErrNOENT;    
//         case ESRCH: return ErrSRCH;    
//         case EINTR: return ErrINTR;    
//         case EIO: return ErrIO;    
//         case ENXIO: return ErrNXIO;    
//         case E2BIG: return Err2BIG;    
//         case ENOEXEC: return ErrNOEXEC;    
//         case EBADF: return ErrBADF;    
//         case ECHILD: return ErrCHILD;   
//         case EAGAIN: return ErrAGAIN;   
//         case ENOMEM: return ErrNOMEM;   
//         case EACCES: return ErrACCES;   
//         case EFAULT: return ErrFAULT;
//         #ifdef ENOTBLK
//             case ENOTBLK: return ErrNOTBLK;
//         #endif
//         case EBUSY: return ErrBUSY;   
//         case EEXIST: return ErrEXIST;   
//         case EXDEV: return ErrXDEV;   
//         case ENODEV: return ErrNODEV;   
//         case ENOTDIR: return ErrNOTDIR;   
//         case EISDIR: return ErrISDIR;   
//         case EINVAL: return ErrINVAL;   
//         case ENFILE: return ErrNFILE;   
//         case EMFILE: return ErrMFILE;   
//         case ENOTTY: return ErrNOTTY;   
//         case ETXTBSY: return ErrTXTBSY;   
//         case EFBIG: return ErrFBIG;   
//         case ENOSPC: return ErrNOSPC;   
//         case ESPIPE: return ErrSPIPE;   
//         case EROFS: return ErrROFS;   
//         case EMLINK: return ErrMLINK;   
//         case EPIPE: return ErrPIPE;   
//         case EDOM: return ErrDOM;   
//         case ERANGE: return ErrRANGE;   

//         case EDEADLK: return ErrDEADLK;   
//         case ENAMETOOLONG: return ErrNAMETOOLONG;  
//         case ENOLCK: return ErrNOLCK;   

//         case ENOSYS: return ErrNOSYS;   

//         case ENOTEMPTY: return ErrNOTEMPTY;  
//         case ELOOP: return ErrLOOP;   
//         //case EWOULDBLOCK: return ErrWOULDBLOCK;  
//         case ENOMSG: return ErrNOMSG;   
//         case EIDRM: return ErrIDRM;  
//         #ifdef ECHRNG
//             case ECHRNG: return ErrCHRNG;
//         #endif
//         #ifdef EL2NSYNC
//             case EL2NSYNC: return ErrL2NSYNC;  
//         #endif
//         #ifdef EL3HLT
//             case EL3HLT: return ErrL3HLT;   
//         #endif
//         #ifdef EL3RST
//             case EL3RST: return ErrL3RST;   
//         #endif
//         #ifdef ELNRNG
//             case ELNRNG: return ErrLNRNG;   
//         #endif
//         #ifdef EUNATCH
//             case EUNATCH: return ErrUNATCH;   
//         #endif
//         #ifdef ENOCSI
//             case ENOCSI: return ErrNOCSI;   
//         #endif
//         #ifdef EL2HLT
//             case EL2HLT: return ErrL2HLT;   
//         #endif
//         #ifdef EBADE
//             case EBADE: return ErrBADE;   
//         #endif
//         #ifdef EBADR
//             case EBADR: return ErrBADR;   
//         #endif
//         #ifdef EXFULL
//             case EXFULL: return ErrXFULL;   
//         #endif
//         #ifdef ENOANO
//             case ENOANO: return ErrNOANO;   
//         #endif
//         #ifdef EBADRQC
//             case EBADRQC: return ErrBADRQC;   
//         #endif
//         #ifdef EBADSLT
//             case EBADSLT: return ErrBADSLT;   
//         #endif

//         //case EDEADLOCK: return ErrDEADLOCK; 

//         #ifdef EBFONT
//             case EBFONT: return ErrBFONT;
//         #endif
//         case ENOSTR: return ErrNOSTR;   
//         case ENODATA: return ErrNODATA;   
//         case ETIME: return ErrTIME;   
//         case ENOSR: return ErrNOSR;   
//         #ifdef ENONET
//             case ENONET: return ErrNONET;   
//         #endif
//         #ifdef ENOPKG
//             case ENOPKG: return ErrNOPKG;   
//         #endif
//         #ifdef EREMOTE
//             case EREMOTE: return ErrREMOTE;   
//         #endif
//         case ENOLINK: return ErrNOLINK;
//         #ifdef EADV
//             case EADV: return ErrADV;
//         #endif
//         #ifdef ESRMNT
//             case ESRMNT: return ErrSRMNT;
//         #endif
//         #ifdef ECOMM
//             case ECOMM: return ErrCOMM;   
//         #endif
//         case EPROTO: return ErrPROTO;   
//         case EMULTIHOP: return ErrMULTIHOP;
//         #ifdef EDOTDOT
//             case EDOTDOT: return ErrDOTDOT;   
//         #endif
//         case EBADMSG: return ErrBADMSG;   
//         case EOVERFLOW: return ErrOVERFLOW; 
//         #ifdef ENOTUNIQ
//             case ENOTUNIQ: return ErrNOTUNIQ; 
//         #endif
//         #ifdef EBADFD
//             case EBADFD: return ErrBADFD;   
//         #endif
//         #ifdef EREMCHG
//         case EREMCHG: return ErrREMCHG;   
//         #endif
//         #ifdef ELIBACC
//             case ELIBACC: return ErrLIBACC;   
//         #endif
//         #ifdef ELIBBAD
//             case ELIBBAD: return ErrLIBBAD;   
//         #endif
//         #ifdef ELIBSCN
//             case ELIBSCN: return ErrLIBSCN;   
//         #endif
//         #ifdef ELIBMAX
//             case ELIBMAX: return ErrLIBMAX;   
//         #endif
//         #ifdef ELIBEXEC
//             case ELIBEXEC: return ErrLIBEXEC;  
//         #endif
//         case EILSEQ: return ErrILSEQ;
//         #ifdef ERESTART
//             case ERESTART: return ErrRESTART;  
//         #endif
//         #ifdef ESTRPIPE
//             case ESTRPIPE: return ErrSTRPIPE;  
//         #endif
//         #ifdef EUSERS
//             case EUSERS: return ErrUSERS;   
//         #endif
//         case ENOTSOCK: return ErrNOTSOCK;  
//         case EDESTADDRREQ: return ErrDESTADDRREQ;  
//         case EMSGSIZE: return ErrMSGSIZE;  
//         case EPROTOTYPE: return ErrPROTOTYPE;  
//         case ENOPROTOOPT: return ErrNOPROTOOPT;  
//         case EPROTONOSUPPORT: return ErrPROTONOSUPPORT;  
//         #ifdef ESOCKTNOSUPPORT
//             case ESOCKTNOSUPPORT: return ErrSOCKTNOSUPPORT;  
//         #endif
//         case EOPNOTSUPP: return ErrOPNOTSUPP;  
//         case EPFNOSUPPORT: return ErrPFNOSUPPORT;  
//         case EAFNOSUPPORT: return ErrAFNOSUPPORT;  
//         case EADDRINUSE: return ErrADDRINUSE;  
//         case EADDRNOTAVAIL: return ErrADDRNOTAVAIL;  
//         case ENETDOWN: return ErrNETDOWN;  
//         case ENETUNREACH: return ErrNETUNREACH;  
//         case ENETRESET: return ErrNETRESET;  
//         case ECONNABORTED: return ErrCONNABORTED;  
//         case ECONNRESET: return ErrCONNRESET;  
//         case ENOBUFS: return ErrNOBUFS;   
//         case EISCONN: return ErrISCONN;   
//         case ENOTCONN: return ErrNOTCONN;
//         #ifdef ESHUTDOWN
//             case ESHUTDOWN: return ErrSHUTDOWN;  
//         #endif
//         case ETOOMANYREFS: return ErrTOOMANYREFS;  
//         case ETIMEDOUT: return ErrTIMEDOUT;  
//         case ECONNREFUSED: return ErrCONNREFUSED;  
//         case EHOSTDOWN: return ErrHOSTDOWN;  
//         case EHOSTUNREACH: return ErrHOSTUNREACH;  
//         case EALREADY: return ErrALREADY;  
//         case EINPROGRESS: return ErrINPROGRESS;  
//         case ESTALE: return ErrSTALE;
//         #ifdef EUCLEAN
//             case EUCLEAN: return ErrUCLEAN;   
//         #endif
//         #ifdef ENOTNAM
//             case ENOTNAM: return ErrNOTNAM;   
//         #endif
//         #ifdef ENAVAIL
//             case ENAVAIL: return ErrNAVAIL;   
//         #endif
//         #ifdef EISNAM
//             case EISNAM: return ErrISNAM;   
//         #endif
//         #ifdef EREMOTEIO
//             case EREMOTEIO: return ErrREMOTEIO;  
//         #endif
//         case EDQUOT: return ErrDQUOT;   
//         #ifdef ENOMEDIUM
//             case ENOMEDIUM: return ErrNOMEDIUM;  
//         #endif
//         #ifdef EMEDIUMTYPE
//             case EMEDIUMTYPE: return ErrMEDIUMTYPE;
//         #endif
//         case ECANCELED: return ErrCANCELED;
//         #ifdef ENOKEY
//             case ENOKEY: return ErrNOKEY;   
//         #endif
//         #ifdef EKEYEXPIRED
//             case EKEYEXPIRED: return ErrKEYEXPIRED;
//         #endif
//         #ifdef EKEYREVOKED
//             case EKEYREVOKED: return ErrKEYREVOKED;  
//         #endif
//         #ifdef EKEYREJECTED
//             case EKEYREJECTED: return ErrKEYREJECTED;  
//         #endif

//         case EOWNERDEAD: return ErrOWNERDEAD;  
//         case ENOTRECOVERABLE: return ErrNOTRECOVERABLE;  

//         #ifdef ERFKILL
//             case ERFKILL: return ErrRFKILL;   
//         #endif

//         #ifdef EHWPOISON
//             case EHWPOISON: return ErrHWPOISON;  
//         #endif
//     }
//     return ErrUnknown;
}
