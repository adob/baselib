#include <errno.h>

#include "error.h"
using namespace lib;

namespace lib::os {
    deferror ErrPERM("Operation not permitted");
    deferror ErrNOENT("No such file or directory");
    deferror ErrSRCH("No such process");
    deferror ErrINTR("Interrupted system call");
    deferror ErrIO("I/O error");
    deferror ErrNXIO("No such device or address");
    deferror Err2BIG("Argument list too long");
    deferror ErrNOEXEC("Exec format error");
    deferror ErrBADF("Bad file number");
    deferror ErrCHILD("No child processes");
    deferror ErrAGAIN("Try again");
    deferror ErrNOMEM("Out of memory");
    deferror ErrACCES("Permission denied");
    deferror ErrFAULT("Bad address");
    deferror ErrNOTBLK("Block device required");
    deferror ErrBUSY("Device or resource busy");
    deferror ErrEXIST("File exists");
    deferror ErrXDEV("Cross-device link");
    deferror ErrNODEV("No such device");
    deferror ErrNOTDIR("Not a directory");
    deferror ErrISDIR("Is a directory");
    deferror ErrINVAL("Invalid argument");
    deferror ErrNFILE("File table overflow");
    deferror ErrMFILE("Too many open files");
    deferror ErrNOTTY("Not a typewriter");
    deferror ErrTXTBSY("Text file busy");
    deferror ErrFBIG("File too large");
    deferror ErrNOSPC("No space left on device");
    deferror ErrSPIPE("Illegal seek");
    deferror ErrROFS("Read-only file system");
    deferror ErrMLINK("Too many links");
    deferror ErrPIPE("Broken pipe");
    deferror ErrDOM("Math argument out of domain of func");
    deferror ErrRANGE("Math result not representable");

    deferror ErrDEADLK("Resource deadlock would occur");
    deferror ErrNAMETOOLONG("File name too long");
    deferror ErrNOLCK("No record locks available");

    deferror ErrNOSYS("Invalid system call number");

    deferror ErrNOTEMPTY("Directory not empty");
    deferror ErrLOOP("Too many symbolic links encountered");
    // deferror ErrWOULDBLOCK("Operation would block");
    deferror ErrNOMSG("No message of desired type");
    deferror ErrIDRM("Identifier removed");
    deferror ErrCHRNG("Channel number out of range");
    deferror ErrL2NSYNC("Level not synchronized");
    deferror ErrL3HLT("Level halted");
    deferror ErrL3RST("Level reset");
    deferror ErrLNRNG("Link number out of range");
    deferror ErrUNATCH("Protocol driver not attached");
    deferror ErrNOCSI("No CSI structure available");
    deferror ErrL2HLT("Level halted");
    deferror ErrBADE("Invalid exchange");
    deferror ErrBADR("Invalid request descriptor");
    deferror ErrXFULL("Exchange full");
    deferror ErrNOANO("No anode");
    deferror ErrBADRQC("Invalid request code");
    deferror ErrBADSLT("Invalid slot");

    // deferror ErrDEADLOCK("Resource deadlock would occur");

    deferror ErrBFONT("Bad font file format");
    deferror ErrNOSTR("Device not a stream");
    deferror ErrNODATA("No data available");
    deferror ErrTIME("Timer expired");
    deferror ErrNOSR("Out of streams resources");
    deferror ErrNONET("Machine is not on the network");
    deferror ErrNOPKG("Package not installed");
    deferror ErrREMOTE("Object is remote");
    deferror ErrNOLINK("Link has been severed");
    deferror ErrADV("Advertise error");
    deferror ErrSRMNT("Srmount error");
    deferror ErrCOMM("Communication error on send");
    deferror ErrPROTO("Protocol error");
    deferror ErrMULTIHOP("Multihop attempted");
    deferror ErrDOTDOT("RFS specific error");
    deferror ErrBADMSG("Not a data message");
    deferror ErrOVERFLOW("Value too large for defined data type");
    deferror ErrNOTUNIQ("Name not unique on network");
    deferror ErrBADFD("File descriptor in bad state");
    deferror ErrREMCHG("Remote address changed");
    deferror ErrLIBACC("Can not access a needed shared library");
    deferror ErrLIBBAD("Accessing a corrupted shared library");
    deferror ErrLIBSCN(".lib section in a.out corrupted");
    deferror ErrLIBMAX("Attempting to link in too many shared libraries");
    deferror ErrLIBEXEC("Cannot exec a shared library directly");
    deferror ErrILSEQ("Illegal byte sequence");
    deferror ErrRESTART("Interrupted system call should be restarted");
    deferror ErrSTRPIPE("Streams pipe error");
    deferror ErrUSERS("Too many users");
    deferror ErrNOTSOCK("Socket operation on non-socket");
    deferror ErrDESTADDRREQ("Destination address required");
    deferror ErrMSGSIZE("Message too long");
    deferror ErrPROTOTYPE("Protocol wrong type for socket");
    deferror ErrNOPROTOOPT("Protocol not available");
    deferror ErrPROTONOSUPPORT("Protocol not supported");
    deferror ErrSOCKTNOSUPPORT("Socket type not supported");
    deferror ErrOPNOTSUPP("Operation not supported on transport endpoint");
    deferror ErrPFNOSUPPORT("Protocol family not supported");
    deferror ErrAFNOSUPPORT("Address family not supported by protocol");
    deferror ErrADDRINUSE("Address already in use");
    deferror ErrADDRNOTAVAIL("Cannot assign requested address");
    deferror ErrNETDOWN("Network is down");
    deferror ErrNETUNREACH("Network is unreachable");
    deferror ErrNETRESET("Network dropped connection because of reset");
    deferror ErrCONNABORTED("Software caused connection abort");
    deferror ErrCONNRESET("Connection reset by peer");
    deferror ErrNOBUFS("No buffer space available");
    deferror ErrISCONN("Transport endpoint is already connected");
    deferror ErrNOTCONN("Transport endpoint is not connected");
    deferror ErrSHUTDOWN("Cannot send after transport endpoint shutdown");
    deferror ErrTOOMANYREFS("Too many references: cannot splice");
    deferror ErrTIMEDOUT("Connection timed out");
    deferror ErrCONNREFUSED("Connection refused");
    deferror ErrHOSTDOWN("Host is down");
    deferror ErrHOSTUNREACH("No route to host");
    deferror ErrALREADY("Operation already in progress");
    deferror ErrINPROGRESS("Operation now in progress");
    deferror ErrSTALE("Stale file handle");
    deferror ErrUCLEAN("Structure needs cleaning");
    deferror ErrNOTNAM("Not a XENIX named type file");
    deferror ErrNAVAIL("No XENIX semaphores available");
    deferror ErrISNAM("Is a named type file");
    deferror ErrREMOTEIO("Remote I/O error");
    deferror ErrDQUOT("Quota exceeded");

    deferror ErrNOMEDIUM("No medium found");
    deferror ErrMEDIUMTYPE("Wrong medium type");
    deferror ErrCANCELED("Operation Canceled");
    deferror ErrNOKEY("Required key not available");
    deferror ErrKEYEXPIRED("Key has expired");
    deferror ErrKEYREVOKED("Key has been revoked");
    deferror ErrKEYREJECTED("Key was rejected by service");

    deferror ErrOWNERDEAD("Owner died");
    deferror ErrNOTRECOVERABLE("State not recoverable");

    deferror ErrRFKILL("Operation not possible due to RF-kill");

    deferror ErrHWPOISON("Memory page has hardware error");
    
    deferror ErrUnknown("Unknown error");
}

deferror &os::from_errno(int n) {
    switch (n) {
        case EPERM: return ErrPERM;    
        case ENOENT: return ErrNOENT;    
        case ESRCH: return ErrSRCH;    
        case EINTR: return ErrINTR;    
        case EIO: return ErrIO;    
        case ENXIO: return ErrNXIO;    
        case E2BIG: return Err2BIG;    
        case ENOEXEC: return ErrNOEXEC;    
        case EBADF: return ErrBADF;    
        case ECHILD: return ErrCHILD;   
        case EAGAIN: return ErrAGAIN;   
        case ENOMEM: return ErrNOMEM;   
        case EACCES: return ErrACCES;   
        case EFAULT: return ErrFAULT;
        #ifdef ENOTBLK
            case ENOTBLK: return ErrNOTBLK;
        #endif
        case EBUSY: return ErrBUSY;   
        case EEXIST: return ErrEXIST;   
        case EXDEV: return ErrXDEV;   
        case ENODEV: return ErrNODEV;   
        case ENOTDIR: return ErrNOTDIR;   
        case EISDIR: return ErrISDIR;   
        case EINVAL: return ErrINVAL;   
        case ENFILE: return ErrNFILE;   
        case EMFILE: return ErrMFILE;   
        case ENOTTY: return ErrNOTTY;   
        case ETXTBSY: return ErrTXTBSY;   
        case EFBIG: return ErrFBIG;   
        case ENOSPC: return ErrNOSPC;   
        case ESPIPE: return ErrSPIPE;   
        case EROFS: return ErrROFS;   
        case EMLINK: return ErrMLINK;   
        case EPIPE: return ErrPIPE;   
        case EDOM: return ErrDOM;   
        case ERANGE: return ErrRANGE;   

        case EDEADLK: return ErrDEADLK;   
        case ENAMETOOLONG: return ErrNAMETOOLONG;  
        case ENOLCK: return ErrNOLCK;   

        case ENOSYS: return ErrNOSYS;   

        case ENOTEMPTY: return ErrNOTEMPTY;  
        case ELOOP: return ErrLOOP;   
        //case EWOULDBLOCK: return ErrWOULDBLOCK;  
        case ENOMSG: return ErrNOMSG;   
        case EIDRM: return ErrIDRM;  
        #ifdef ECHRNG
            case ECHRNG: return ErrCHRNG;
        #endif
        #ifdef EL2NSYNC
            case EL2NSYNC: return ErrL2NSYNC;  
        #endif
        #ifdef EL3HLT
            case EL3HLT: return ErrL3HLT;   
        #endif
        #ifdef EL3RST
            case EL3RST: return ErrL3RST;   
        #endif
        #ifdef ELNRNG
            case ELNRNG: return ErrLNRNG;   
        #endif
        #ifdef EUNATCH
            case EUNATCH: return ErrUNATCH;   
        #endif
        #ifdef ENOCSI
            case ENOCSI: return ErrNOCSI;   
        #endif
        #ifdef EL2HLT
            case EL2HLT: return ErrL2HLT;   
        #endif
        #ifdef EBADE
            case EBADE: return ErrBADE;   
        #endif
        #ifdef EBADR
            case EBADR: return ErrBADR;   
        #endif
        #ifdef EXFULL
            case EXFULL: return ErrXFULL;   
        #endif
        #ifdef ENOANO
            case ENOANO: return ErrNOANO;   
        #endif
        #ifdef EBADRQC
            case EBADRQC: return ErrBADRQC;   
        #endif
        #ifdef EBADSLT
            case EBADSLT: return ErrBADSLT;   
        #endif

        //case EDEADLOCK: return ErrDEADLOCK; 

        #ifdef EBFONT
            case EBFONT: return ErrBFONT;
        #endif
        case ENOSTR: return ErrNOSTR;   
        case ENODATA: return ErrNODATA;   
        case ETIME: return ErrTIME;   
        case ENOSR: return ErrNOSR;   
        #ifdef ENONET
            case ENONET: return ErrNONET;   
        #endif
        #ifdef ENOPKG
            case ENOPKG: return ErrNOPKG;   
        #endif
        #ifdef EREMOTE
            case EREMOTE: return ErrREMOTE;   
        #endif
        case ENOLINK: return ErrNOLINK;
        #ifdef EADV
            case EADV: return ErrADV;
        #endif
        #ifdef ESRMNT
            case ESRMNT: return ErrSRMNT;
        #endif
        #ifdef ECOMM
            case ECOMM: return ErrCOMM;   
        #endif
        case EPROTO: return ErrPROTO;   
        case EMULTIHOP: return ErrMULTIHOP;
        #ifdef EDOTDOT
            case EDOTDOT: return ErrDOTDOT;   
        #endif
        case EBADMSG: return ErrBADMSG;   
        case EOVERFLOW: return ErrOVERFLOW; 
        #ifdef ENOTUNIQ
            case ENOTUNIQ: return ErrNOTUNIQ; 
        #endif
        #ifdef EBADFD
            case EBADFD: return ErrBADFD;   
        #endif
        #ifdef EREMCHG
        case EREMCHG: return ErrREMCHG;   
        #endif
        #ifdef ELIBACC
            case ELIBACC: return ErrLIBACC;   
        #endif
        #ifdef ELIBBAD
            case ELIBBAD: return ErrLIBBAD;   
        #endif
        #ifdef ELIBSCN
            case ELIBSCN: return ErrLIBSCN;   
        #endif
        #ifdef ELIBMAX
            case ELIBMAX: return ErrLIBMAX;   
        #endif
        #ifdef ELIBEXEC
            case ELIBEXEC: return ErrLIBEXEC;  
        #endif
        case EILSEQ: return ErrILSEQ;
        #ifdef ERESTART
            case ERESTART: return ErrRESTART;  
        #endif
        #ifdef ESTRPIPE
            case ESTRPIPE: return ErrSTRPIPE;  
        #endif
        #ifdef EUSERS
            case EUSERS: return ErrUSERS;   
        #endif
        case ENOTSOCK: return ErrNOTSOCK;  
        case EDESTADDRREQ: return ErrDESTADDRREQ;  
        case EMSGSIZE: return ErrMSGSIZE;  
        case EPROTOTYPE: return ErrPROTOTYPE;  
        case ENOPROTOOPT: return ErrNOPROTOOPT;  
        case EPROTONOSUPPORT: return ErrPROTONOSUPPORT;  
        #ifdef ESOCKTNOSUPPORT
            case ESOCKTNOSUPPORT: return ErrSOCKTNOSUPPORT;  
        #endif
        case EOPNOTSUPP: return ErrOPNOTSUPP;  
        case EPFNOSUPPORT: return ErrPFNOSUPPORT;  
        case EAFNOSUPPORT: return ErrAFNOSUPPORT;  
        case EADDRINUSE: return ErrADDRINUSE;  
        case EADDRNOTAVAIL: return ErrADDRNOTAVAIL;  
        case ENETDOWN: return ErrNETDOWN;  
        case ENETUNREACH: return ErrNETUNREACH;  
        case ENETRESET: return ErrNETRESET;  
        case ECONNABORTED: return ErrCONNABORTED;  
        case ECONNRESET: return ErrCONNRESET;  
        case ENOBUFS: return ErrNOBUFS;   
        case EISCONN: return ErrISCONN;   
        case ENOTCONN: return ErrNOTCONN;
        #ifdef ESHUTDOWN
            case ESHUTDOWN: return ErrSHUTDOWN;  
        #endif
        case ETOOMANYREFS: return ErrTOOMANYREFS;  
        case ETIMEDOUT: return ErrTIMEDOUT;  
        case ECONNREFUSED: return ErrCONNREFUSED;  
        case EHOSTDOWN: return ErrHOSTDOWN;  
        case EHOSTUNREACH: return ErrHOSTUNREACH;  
        case EALREADY: return ErrALREADY;  
        case EINPROGRESS: return ErrINPROGRESS;  
        case ESTALE: return ErrSTALE;
        #ifdef EUCLEAN
            case EUCLEAN: return ErrUCLEAN;   
        #endif
        #ifdef ENOTNAM
            case ENOTNAM: return ErrNOTNAM;   
        #endif
        #ifdef ENAVAIL
            case ENAVAIL: return ErrNAVAIL;   
        #endif
        #ifdef EISNAM
            case EISNAM: return ErrISNAM;   
        #endif
        #ifdef EREMOTEIO
            case EREMOTEIO: return ErrREMOTEIO;  
        #endif
        case EDQUOT: return ErrDQUOT;   
        #ifdef ENOMEDIUM
            case ENOMEDIUM: return ErrNOMEDIUM;  
        #endif
        #ifdef EMEDIUMTYPE
            case EMEDIUMTYPE: return ErrMEDIUMTYPE;
        #endif
        case ECANCELED: return ErrCANCELED;
        #ifdef ENOKEY
            case ENOKEY: return ErrNOKEY;   
        #endif
        #ifdef EKEYEXPIRED
            case EKEYEXPIRED: return ErrKEYEXPIRED;
        #endif
        #ifdef EKEYREVOKED
            case EKEYREVOKED: return ErrKEYREVOKED;  
        #endif
        #ifdef EKEYREJECTED
            case EKEYREJECTED: return ErrKEYREJECTED;  
        #endif

        case EOWNERDEAD: return ErrOWNERDEAD;  
        case ENOTRECOVERABLE: return ErrNOTRECOVERABLE;  

        #ifdef ERFKILL
            case ERFKILL: return ErrRFKILL;   
        #endif

        #ifdef EHWPOISON
            case EHWPOISON: return ErrHWPOISON;  
        #endif
    }
    return ErrUnknown;
}
