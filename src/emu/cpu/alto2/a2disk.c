/*****************************************************************************
 *
 *   Portable Xerox AltoII disk interface
 *
 *   Copyright: Juergen Buchmueller <pullmoll@t-online.de>
 *
 *   Licenses: MAME, GPLv2
 *
 *****************************************************************************/
#include "alto2.h"


#define	JKFF_FUNCTION	1	//!< define 1 to debug the JK flip-flops, 0 to use a lookup table

#define	GET_KADDR_SECTOR(kaddr)			A2_GET16(kaddr,16, 0, 3)			//!< get sector number from address register
#define	PUT_KADDR_SECTOR(kaddr,val)		A2_PUT16(kaddr,16, 0, 3,val)		//!< put sector number into address register
#define	GET_KADDR_CYLINDER(kaddr)		A2_GET16(kaddr,16, 4,12)			//!< get cylinder number from address register
#define	PUT_KADDR_CYLINDER(kaddr,val)	A2_PUT16(kaddr,16, 4,12,val)		//!< put cylinder number int address register
#define	GET_KADDR_HEAD(kaddr)			A2_GET16(kaddr,16,13,13)			//!< get head number from address register
#define	PUT_KADDR_HEAD(kaddr,val)		A2_PUT16(kaddr,16,13,13,val)		//!< put head number into address register
#define	GET_KADDR_DRIVE(kaddr)			A2_GET16(kaddr,16,14,14)			//!< get drive (unit) number from address register
#define	PUT_KADDR_DRIVE(kaddr,val)		A2_PUT16(kaddr,16,14,14,val)		//!< put drive (unit) number into address register
#define	GET_KADDR_RESTORE(kaddr)		A2_GET16(kaddr,16,15,15)			//!< get restore flag from address register
#define	PUT_KADDR_RESTORE(kaddr,val)	A2_PUT16(kaddr,16,15,15,val)		//!< putt restore flag into address register

#define	GET_KADR_SEAL(kadr)				A2_GET16(kadr,16, 0, 7)				//!< get command seal from command register
#define	PUT_KADR_SEAL(kadr,val)			A2_PUT16(kadr,16, 0, 7,val)			//!< put command seal into command register
#define	GET_KADR_HEADER(kadr)			A2_GET16(kadr,16, 8, 9)				//!< get r/w/c for header from command register
#define	PUT_KADR_HEADER(kadr,val)		A2_PUT16(kadr,16, 8, 9,val)			//!< put r/w/c for header from command register
#define	GET_KADR_LABEL(kadr)			A2_GET16(kadr,16,10,11)				//!< get r/w/c for label from command register
#define	PUT_KADR_LABEL(kadr,val)		A2_PUT16(kadr,16,10,11,val)			//!< put r/w/c for label into command register
#define	GET_KADR_DATA(kadr)				A2_GET16(kadr,16,12,13)				//!< get r/w/c for data from command register
#define	PUT_KADR_DATA(kadr,val)			A2_PUT16(kadr,16,12,13,val)			//!< put r/w/c for data into command register
#define	GET_KADR_NOXFER(kadr)			A2_GET16(kadr,16,14,14)				//!< get no transfer flag from command register
#define	PUT_KADR_NOXFER(kadr,val)		A2_PUT16(kadr,16,14,14,val)			//!< put no transfer flag into command register
#define	GET_KADR_UNUSED(kadr)			A2_GET16(kadr,16,15,15)				//!< get unused (drive?) flag from command register
#define	PUT_KADR_UNUSED(kadr,val)		A2_PUT16(kadr,16,15,15,val)			//!< put unused (drive?) flag into command register

#define	GET_KSTAT_SECTOR(kstat)			A2_GET16(kstat,16,0,3)				//!< get current sector number from status register
#define	PUT_KSTAT_SECTOR(kstat,val)		A2_PUT16(kstat,16,0,3,val)			//!< put current sector number into status register
#define	GET_KSTAT_DONE(kstat)			A2_GET16(kstat,16,4,7)				//!< get 'done' field from status register (017)
#define	PUT_KSTAT_DONE(kstat,val)		A2_PUT16(kstat,16,4,7,val)			//!< put 'done' field int status register (017)
#define	GET_KSTAT_SEEKFAIL(kstat)		A2_GET16(kstat,16,8,8)				//!< get seek fail flag from status register
#define	PUT_KSTAT_SEEKFAIL(kstat,val)	A2_PUT16(kstat,16,8,8,val)			//!< put seek fail flag into status register
#define	GET_KSTAT_SEEK(kstat)			A2_GET16(kstat,16,9,9)				//!< get seek busy flag (strobe) from status register
#define	PUT_KSTAT_SEEK(kstat,val)		A2_PUT16(kstat,16,9,9,val)			//!< put seek busy flag (strobe) into status register
#define	GET_KSTAT_NOTRDY(kstat)			A2_GET16(kstat,16,10,10)			//!< get drive not ready flag from status register
#define	PUT_KSTAT_NOTRDY(kstat,val)		A2_PUT16(kstat,16,10,10,val)		//!< put drive not ready flag into status register
#define	GET_KSTAT_DATALATE(kstat)		A2_GET16(kstat,16,11,11)			//!< get data late flag from status register
#define	PUT_KSTAT_DATALATE(kstat,val)	A2_PUT16(kstat,16,11,11,val)		//!< put data late flag into status register
#define	GET_KSTAT_IDLE(kstat)			A2_GET16(kstat,16,12,12)			//!< get idle flag from status register (idle is a software flag)
#define	PUT_KSTAT_IDLE(kstat,val)		A2_PUT16(kstat,16,12,12,val)		//!< put idle flag into status register (idle is a software flag)
#define	GET_KSTAT_CKSUM(kstat)			A2_GET16(kstat,16,13,13)			//!< get checksum flag from status register (checksum is a software flag; it is ORed when 0)
#define	PUT_KSTAT_CKSUM(kstat,val)		A2_PUT16(kstat,16,13,13,val)		//!< put checksum flag into status register (checksum is a software flag; it is ORed when 0)
#define	GET_KSTAT_COMPLETION(kstat)		A2_GET16(kstat,16,14,15)			//!< get completion code from status register (completion is a 2-bit software latch)
#define	PUT_KSTAT_COMPLETION(kstat,val)	A2_PUT16(kstat,16,14,15,val)		//!< put completion code into status register (completion is a 2-bit software latch)

#define	GET_KCOM_XFEROFF(kcom)			A2_GET16(kcom,16,1,1)				//!< get transfer off flag from controller command (hardware command register)
#define	PUT_KCOM_XFEROFF(kcom,val)		A2_PUT16(kcom,16,1,1,val)			//!< put transfer off flag into controller command (hardware command register)
#define	GET_KCOM_WDINHIB(kcom)			A2_GET16(kcom,16,2,2)				//!< get word task inhibit flag from controller command (hardware command register)
#define	PUT_KCOM_WDINHIB(kcom,val)		A2_PUT16(kcom,16,2,2,val)			//!< put word task inhibit flag into controller command (hardware command register)
#define	GET_KCOM_BCLKSRC(kcom)			A2_GET16(kcom,16,3,3)				//!< get bit clock source flag from controller command (hardware command register)
#define	PUT_KCOM_BCLKSRC(kcom,val)		A2_PUT16(kcom,16,3,3,val)			//!< put bit clock source flag into controller command (hardware command register)
#define	GET_KCOM_WFFO(kcom)				A2_GET16(kcom,16,4,4)				//!< get write fixed frequency oscillator flag from controller command (hardware command register)
#define	PUT_KCOM_WFFO(kcom,val)			A2_PUT16(kcom,16,4,4,val)			//!< put write fixed frequency oscillator flag into controller command (hardware command register)
#define	GET_KCOM_SENDADR(kcom)			A2_GET16(kcom,16,5,5)				//!< get send address flag from controller command (hardware command register)
#define	PUT_KCOM_SENDADR(kcom,val)		A2_PUT16(kcom,16,5,5,val)			//!< put send address flag into controller command (hardware command register)

/**
 * @brief callback is called by the drive timer whenever a new sector starts
 *
 * @param unit the unit number
 */
static void disk_sector_start(void* cookie, int unit)
{
	alto2_cpu_device* cpu = reinterpret_cast<alto2_cpu_device *>(cookie);
	cpu->next_sector(unit);
}

/** @brief completion codes (only for documentation, since this is microcode defined) */
enum {
	STATUS_COMPLETION_GOOD,
	STATUS_COMPLETION_HARDWARE_ERROR,
	STATUS_COMPLETION_CHECK_ERROR,
	STATUS_COMPLETION_ILLEGAL_SECTOR
};

/** @brief record numbers per sector in INCRECNO order */
enum {
	RECNO_HEADER,
	RECNO_PAGENO,
	RECNO_LABEL,
	RECNO_DATA
};


/** @brief read/write/check numbers */
enum {
	RWC_READ,
	RWC_CHECK,
	RWC_WRITE,
	RWC_WRITE2
};

#if	ALTO2_DEBUG
/** @brief human readable names for the KADR← modes */
static const char *rwc_name[4] = {"read", "check", "write", "write2"};
#endif

#if	ALTO2_DEBUG
static const char* raise_lower[2] = {"↗","↘"};
static const char* jkff_name;
/** @brief macro to set the name of a FF in DEBUG=1 builds only */
#define	DEBUG_NAME(x)	jkff_name = x
#else
#define	DEBUG_NAME(x)
#endif

#if	JKFF_FUNCTION

/**
 * @brief simulate a 74109 J-K flip-flop with set and reset inputs
 *
 * @param s0 is the previous state of the FF's in- and outputs
 * @param s1 is the next state
 * @return returns the next state and probably modified Q output
 */
jkff_t alto2_cpu_device::update_jkff(UINT8 s0, UINT8 s1)
{
	switch (s1 & (JKFF_C | JKFF_S)) {
	case JKFF_C | JKFF_S:	/* C' is 1, and S' is 1 */
		if (((s0 ^ s1) & s1) & JKFF_CLK) {
			/* rising edge of the clock */
			switch (s1 & (JKFF_J | JKFF_K)) {
			case 0:
				/* both J and K' are 0: set Q to 0, Q' to 1 */
				s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
				if (s0 & JKFF_Q) {
					LOG((LOG_DISK,9,"%s J:0 K':0 → Q:0\n", jkff_name));
				}
				break;
			case JKFF_J:
				/* J is 1, and K' is 0: toggle Q */
				if (s0 & JKFF_Q)
					s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
				else
					s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
				LOG((LOG_DISK,9,"%s J:0 K':1 flip-flop Q:%d\n", jkff_name, (s1 & JKFF_Q) ? 1 : 0));
				break;
			case JKFF_K:
				if ((s0 ^ s1) & JKFF_Q) {
					LOG((LOG_DISK,9,"%s J:0 K':1 keep Q:%d\n", jkff_name, (s1 & JKFF_Q) ? 1 : 0));
				}
				/* J is 0, and K' is 1: keep Q as is */
				if (s0 & JKFF_Q)
					s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
				else
					s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
				break;
			case JKFF_J | JKFF_K:
				/* both J and K' are 1: set Q to 1 */
				s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
				if (!(s0 & JKFF_Q)) {
					LOG((LOG_DISK,9,"%s J:1 K':1 → Q:1\n", jkff_name));
				}
				break;
			}
		} else {
			/* keep Q */
			s1 = (s1 & ~JKFF_Q) | (s0 & JKFF_Q);
		}
		break;
	case JKFF_S:
		/* S' is 1, C' is 0: set Q to 0, Q' to 1 */
		s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
		if (s0 & JKFF_Q) {
			LOG((LOG_DISK,9,"%s C':0 → Q:0\n", jkff_name));
		}
		break;
	case JKFF_C:
		/* S' is 0, C' is 1: set Q to 1, Q' to 0 */
		s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
		if (!(s0 & JKFF_Q)) {
			LOG((LOG_DISK,9,"%s S':0 → Q:1\n", jkff_name));
		}
		break;
	case 0:
	default:
		/* unstable state (what to do?) */
		s1 = s1 | JKFF_Q | JKFF_Q0;
		LOG((LOG_DISK,9,"%s C':0 S':0 → Q:1 and Q':1 <unstable>\n", jkff_name));
		break;
	}
	return static_cast<jkff_t>(s1);
}

#else

/** @brief table constructed for update_jkff(s0,s1) */
static UINT8 jkff_lookup[64][64] = {
	{
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60
	},{
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61
	},{
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62
	},{
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63
	},{
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64
	},{
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65
	},{
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66
	},{
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67
	},{
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48
	},{
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49
	},{
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a
	},{
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b
	},{
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c
	},{
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d
	},{
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e
	},{
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f
	},{
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30
	},{
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31
	},{
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32
	},{
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33
	},{
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34
	},{
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35
	},{
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36
	},{
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37
	},{
		0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
		0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
		0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
		0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
		0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,
		0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,
		0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,
		0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38
	},{
		0x59,0x19,0x59,0x19,0x59,0x19,0x59,0x19,
		0x59,0x19,0x59,0x19,0x59,0x19,0x59,0x19,
		0x59,0x19,0x59,0x19,0x59,0x19,0x59,0x19,
		0x59,0x19,0x59,0x19,0x59,0x19,0x59,0x19,
		0x59,0x39,0x59,0x39,0x59,0x39,0x59,0x39,
		0x59,0x39,0x59,0x39,0x59,0x39,0x59,0x39,
		0x59,0x39,0x59,0x39,0x59,0x39,0x59,0x39,
		0x59,0x39,0x59,0x39,0x59,0x39,0x59,0x39
	},{
		0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,
		0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,
		0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,
		0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,
		0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,
		0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,
		0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,
		0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a
	},{
		0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,
		0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,
		0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,
		0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,
		0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,
		0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,
		0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,
		0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b
	},{
		0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
		0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
		0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
		0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
		0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,
		0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,
		0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,
		0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c
	},{
		0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,
		0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,
		0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,
		0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,
		0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,
		0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,
		0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,
		0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d
	},{
		0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
		0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
		0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
		0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
		0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,
		0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,
		0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,
		0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e
	},{
		0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
		0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
		0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
		0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
		0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
		0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
		0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
		0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f
	},{
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
		0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60
	},{
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61,
		0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x61
	},{
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
		0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62
	},{
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
		0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63
	},{
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
		0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64
	},{
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,
		0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65
	},{
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
		0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66
	},{
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67,
		0x67,0x67,0x67,0x67,0x67,0x67,0x67,0x67
	},{
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
		0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48
	},{
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,
		0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49
	},{
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
		0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a
	},{
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,
		0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b,0x4b
	},{
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
		0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c
	},{
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
		0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d
	},{
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,
		0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e
	},{
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,
		0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f,0x4f
	},{
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
		0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30
	},{
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,
		0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31
	},{
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32
	},{
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
		0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33
	},{
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,
		0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34
	},{
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
		0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35
	},{
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
		0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36
	},{
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,
		0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37
	},{
		0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
		0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
		0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
		0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
		0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,
		0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,
		0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,
		0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38
	},{
		0x59,0x19,0x59,0x19,0x59,0x19,0x59,0x19,
		0x59,0x19,0x59,0x19,0x59,0x19,0x59,0x19,
		0x59,0x19,0x59,0x19,0x59,0x19,0x59,0x19,
		0x59,0x19,0x59,0x19,0x59,0x19,0x59,0x19,
		0x59,0x39,0x59,0x39,0x59,0x39,0x59,0x39,
		0x59,0x39,0x59,0x39,0x59,0x39,0x59,0x39,
		0x59,0x39,0x59,0x39,0x59,0x39,0x59,0x39,
		0x59,0x39,0x59,0x39,0x59,0x39,0x59,0x39
	},{
		0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,
		0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,
		0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,
		0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,
		0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,
		0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,
		0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,
		0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a
	},{
		0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,
		0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,
		0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,
		0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,0x3b,0x1b,
		0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,
		0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,
		0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,
		0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b
	},{
		0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
		0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
		0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
		0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
		0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,
		0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,
		0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,
		0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c
	},{
		0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,
		0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,
		0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,
		0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,0x5d,0x1d,
		0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,
		0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,
		0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,
		0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d
	},{
		0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
		0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
		0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
		0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
		0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,
		0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,
		0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,
		0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e
	},{
		0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
		0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
		0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
		0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
		0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
		0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
		0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
		0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f
	}
};

jkff_t alto2_cpu_device::update_jkff(UINT8 s0, UINT8 s1)
{
	UINT8 result = jkff_lookup[s1 & 63][ s0 & 63];
#if	ALTO2_DEBUG
	LOG((LOG_DISK,9,"%s : ", jkff_name));
	if ((s0 ^ result) & JKFF_CLK)
		LOG((LOG_DISK,9," CLK%s", raise_lower[result & 1]));
	if ((s0 ^ result) & JKFF_J)
		LOG((LOG_DISK,9," J%s", raise_lower[(result >> 1) & 1]));
	if ((s0 ^ result) & JKFF_K)
		LOG((LOG_DISK,9," K'%s", raise_lower[(result >> 2) & 1]));
	if ((s0 ^ result) & JKFF_S)
		LOG((LOG_DISK,9," S'%s", raise_lower[(result >> 3) & 1]));
	if ((s0 ^ result) & JKFF_C)
		LOG((LOG_DISK,9," C'%s", raise_lower[(result >> 4) & 1]));
	if ((s0 ^ result) & JKFF_Q)
		LOG((LOG_DISK,9," Q%s", raise_lower[(result >> 5) & 1]));
	if ((s0 ^ result) & JKFF_Q0)
		LOG((LOG_DISK,9," Q'%s", raise_lower[(result >> 6) & 1]));
	LOG((LOG_DISK,9,"\n"));
#endif
	return static_cast<jkff_t>(result);
}
#endif

/**
 * <PRE>
 * SECTOR, ERROR WAKEUPS
 *
 *
 * Monoflop pulse duration:
 * tW = K * Rt * Cext * (1 + 0.7/Rt)
 * K = 0.28 for 74123
 * Rt = kOhms
 * Cext = pF
 *
 *                     +------+
 *  CLRSTAT' >---------oS'    | 15k, .47µF (=470000pF)
 *                     | MONO | 2066120ns ~= 2ms
 *                     | FLOP |
 *                     |      | Q'     +----+
 *  READY'   >---------oC'    o--------|NAND|    ERRWAKE'
 *                     +------+        |    o----·
 *  RDYLAT'  >-------------------------|    |    |
 *                                     +----+    |
 *                                               |
 *                                               |
 *                         .---------------------·
 *                         |
 *                     +---o--+ Q            +------+
 *               ·-----|J  S' |----+---------|S     | 30k, .01µF (=10000pF)
 *               |     |      |    |         | MONO | 85960ns ~= 86µs
 *   SECT[4] >---|-----|CLK   |    |         | FLOP |
 *               |     |   21a|    |         |      | Q'
 *               | 1 >-|K' C' |    |     1 >-|C'    |--------------------> SECLATE
 *               |     +---o--+    |         +------+
 *               |         |       |
 *               ·---------+-------|-----------------------------------.
 *                                 |                                   |
 *                 ·---------------·                                   |
 *                 |                                                   |
 *                 |       1                 1   RESET' >------·       |
 *                 |       |                 |                 |       |
 *                 |   +---o--+ Q        +---o--+ Q        +---o--+ Q  |
 *                 ·---|J  S' |----------|J  S' |----------|J  S' |------> STSKENA
 *                     |      |          |      |          |      |    |
 *  SYSCLKB' >--+------|CLK   |  .-------|CLK   |  ·-------|CLK   |    |
 *              |      |   21b|  |       |   22a|  |       |   22b| Q' |
 *              |  1 >-|K' C' |  |   1 >-|K' C' |  |   ·---|K' C' |----+-> WAKEST'
 *              |      +---o--+  |       +---o--+  |   |   +---o--+    |
 *              |          |     |           |     |   |       1       |
 *              |          ·-----|-----------+-----|---|---------------·
 *              |                |                 |   |
 *              ·----------------+-----------------·   |
 *                                                     |
 *                                     +----+          |
 *   BLOCK   >-------------------------|NAND|          |
 *                                     |    o----------·
 *  STSKACT  >-------------------------|    |
 *                                     +----+
 *
 * A CLRSTAT starts the monoflop, and READY', i.e. the ready signal from the disk
 * drive, clears it. The Q' output is thus 0 for some time after CLRSTAT, and as
 * long as the disk signals being ready.
 *
 * If the disk is not ready, i.e. the Q' being 1, and if RDYLAT' - the READY' state
 * latched at the most recent CLRSTAT - is also 1, the ERRWAKE' signal will go 0.
 *
 * Each new sector (SECT[4]' going 1) will clock the FF 21a, which changes
 * its Q output depending on WAKEST' (K' is always 1):
 *   if J and K' are both 1, sets its Q to 1.
 *   if J is 0, and K' is 1, keeps Q as is.
 * So Q becomes 0 by WAKEST' going 0, and it becomes 1 with the next sector, if
 * WAKEST' is 1.
 *
 * The mono-flop to the right will generate a SECLATE signal, if WAKEST' was
 * not 0 when the disk signalled a new sector.
 *
 * The three J-K FFs at the bottom are all clocked with the rising edge of
 * SYSCLKB' (i.e falling edge of SYSCLKB).
 *
 * The left JK-FF propagates the current state of the upper JK-FF's Q output
 * to its own Q. The middle propagates the previous state of the left one,
 * and the JK-FF to the right delays the wandering Q for a third SYSCLKB'
 * rising edge, but only in one case:
 * 1)  if J and K' are both 1, set its Q to 1.
 * 2)  if J is 1, and K' is 0, toggle Q.
 * 3)  if J is 0, and K' is 1, keep Q as is.
 * 4)  if J and K' are both 0, set its Q to 0.
 *
 * The right FF's K' is 0 whenever the BLOCK signal (see DISK WORD TIMING)
 * and the sector task active signal (STSKACT) are 1 at the same time.
 *
 * Case 1) is the normal case, and it wakes the KSECT on the third SYSCLKB'
 * positive edge. It resets at that same time the left, middle, and upper
 * J-K FFs .
 *
 * Case 2) is due, when the sector task is already active the moment
 * the BLOCK signal arrives. This toggles the output, i.e. removes the
 * wake.
 *
 * Case 3) is for an active sector task without a new sector.
 *
 * And finally case 4) happens when an active sector task sees no new
 * sector, and BLOCK rises.
 *
 * (This is like the video timing's dwt_blocks and dht_blocks signals)
 * </PRE>
 */

/**
 * @brief monoflop 31a pulse duration
 * Rt = 15k, Cext = .47µF (=470000pF) => 2066120ns (~2ms)
 */
#define	TW_READY	2066120

/**
 * @brief monoflop 31b pulse duration
 * Rt = 30k, Cext = .01µF (=10000pF) => 86960ns (~85us)
 *
 * There's something wrong with this, or the KSEC would never ever
 * be able to commence the KWD. The SECLATE monoflop ouput has to go
 * high some time into the KSEC task microcode, before the sequence
 * error state is checked.
 *
 *  TW_SECLATE	(85960 nsec)
 *  TW_SECLATE	(46*ALTO2_UCYCLE)
*/
#define	TW_SECLATE	8596

/** @brief monoflop 52b pulse duration
 * Rt = 20k, Cext = 0.01µF (=10000pF) => 57960ns (~58us)
 */
#define	TW_STROBON	57960

/**
 * <PRE>
 * DISK WORD TIMING
 *
 *
 *                       SECLATE ----·  +-+-+-+---< 1
 *                                   |  | | | |
 *                                +--o-----------+ CARRY +---+
 *                                | CLR 1 2 4 8  |-------|INVo-----> WDDONE'
 *                +---+  BITCLK'  |              |       +---+
 *    BITCLK >----|INVo----+------|CLK/   74161  |
 *                +---+    |      +----o---------+
 *                         |           |LOAD'
 *              ·----------·           |
 *              |  +----+              |
 *              ·--|NAND|              |
 *                 |    o----·         |
 *  HIORDBIT >-----|    |    |         |
 *                 +----+    |         |
 *                           |         |
 *                       +---o--+ Q    |
 *    BUS[4] >--------+--|J  S' |------·
 *                    |  |      |                               +----+
 *    LDCOM' >--------|--|CLK   |                        ·------|NAND|
 *                    |  |   67b|                        |      |    o----> WAKEWDT'
 *                    +--|K' C' |                        |   ·--|    |
 *                       +---o--+                        |   |  +----+
 *                           |                           |   |
 *                           1                           |   ·----------·
 *                                                       |              |
 * OK TO RUN >---------------·                 1         |       1      |
 * (1 in AltoI)              |                 |         |       |      |
 *                       +---o--+ Q        +---o--+ Q    |   +---o--+ Q |
 *                   1 >-|J  S' |----------|J  S' |------+---|J  S' |---·
 *                       |      |          |      |      |   |      |
 *   WDDONE' >-----------|CLK   |  ·-------|CLK   |   .--|---|CLK   |
 *                       |   43b|  |       |   53a|   |  |   |   43a|
 *                   1 >-|K' C' |  | ·-----|K' C' |   |  `---|K' C' o---·
 *                       +---o--+  | |     +---o--+   |      +---o--+ Q'|
 *                           |     | |         |      |          |      |
 *                           ·-----|-|---------|------|----------|------·
 *                                 | |         |      |          |
 *  SYSCLKA' >---------------------|-|---------|------·          |
 *                                 | |         |                 |
 *  WDALLOW  >---------------------|-|---------+-----------------·
 *                                 | |         |
 *  SYSCLKB' >---------------------+ |     +---o--+ Q
 *                                 | | 0 >-|J  S' |-------------> WDINIT
 *                                 | |     |      |
 *              +----+             ·-|-----|CLK   |
 *     BLOCK >--|NAND|               |     |   53b|
 *              |    o---------------+-----|K  C' |
 *  WDTSKACT >--|    |                     +---o--+
 *              +----+                         |
 *                                             1
 *
 *
 * If SECLATE is 0, WDDONE' never goes low (counter's clear has precedence).
 *
 * If SECLATE is 1, WDDONE', the counter will count:
 *
 * If HIORDBIT is 1 at the falling edge of BITCLK, it sets the J-K flip-flop 67b, and
 * thus takes away the LOAD' assertion from the counter. It has been loaded with
 * 15, so it counts to 16 on the next rising edge and makes WDDONE' go to 0.
 *
 * If HIORDBIT is 0 at the falling edge of BITCLK, counting continues as it was
 * preset with BUS[4] at the last KCOM← load:
 *
 * If BUS[4] was 1, both J and K' of the FF (74109) will be 1 at the rising edge
 * of LDCOM' (at the end of KCOM←) and Q will be 1 => LOAD' is deasserted.
 *
 * If BUS[4] was 0, both J and K' will be 0, and Q will be 0 => LOAD' asserted.
 *
 * WDDONE' going from 0 to 1 will make the Q output of FF 43b go to 1.
 * The FF is also set, as long as OK TO RUN is 0.
 *
 * The FF 53a is clocked with falling edge of SYSCLKB (rising of SYSCLKB'),
 * and will:
 *   if J and K' are both 1, set its Q to 1.
 *   if J is 1, and K' is 0, toggle Q.
 *   if J is 0, and K' is 1, keep Q as is.
 *   if J and K' are both 0, set its Q to 0.
 * J is = Q of the FF 43b.
 * K is = 0, if both BLOCK and WDTASKACT are 1, and 1 otherwise.
 *
 * The FF 43a is clocked with falling edge of SYSCLKA (rising of SYSCLKA').
 * Its J and K' inputs are the Q output of the previous (middle) FF, thus
 * it will propagate the middle FF's Q to its own Q when SYSCLKA goes 0.
 *
 * If Q (53a) and Q (43a) are both 1, the WAKEKWDT' is 0 and the
 * word task wakeup signal is sent.
 *
 * WDALLOW going 0 asynchronously resets the 53a and 43a FFs, and thus
 * deasserts the WAKEKWD'. It also asynchronously sets the FF 53b, and
 * its output Q is the WDINIT signal.
 *
 * WDINIT is also deasserted with SYSCLKB going high, whenever both BLOCK
 * and WDTSKACT are 1.
 *
 * Whoa there! :-)
 * </PRE>
 */
#define	INIT	(m_task == task_kwd && m_dsk.wdinit0)

#define	WDALLOW	(!GET_KCOM_WDINHIB(m_dsk.kcom))
#define	WDINIT	((m_dsk.ff_53b & JKFF_Q) ? 1 : 0)
#define	RDYLAT	((m_dsk.ff_45a & JKFF_Q) ? 1 : 0)
#define	SEQERR	((m_task == task_ksec && m_dsk.seclate == 0) || (m_task == task_kwd && m_dsk.carry == 1))
#define	ERRWAKE	(RDYLAT | m_dsk.ready_mf31a)
#define	SEEKOK	(m_dsk.seekok)

/**
 * @brief disk word timing
 *
 * Implement the FIFOs and gates in the description above.
 *
 * @param bitclk the current bitclk level
 * @param datin the level of the bit read from the disk
 * @param block contains the task number of a blocking task, or 0 otherwise
 */
void alto2_cpu_device::kwd_timing(int bitclk, int datin, int block)
{
	diablo_hd_device* dhd = m_drive[m_dsk.drive];
	int wddone = m_dsk.wddone;		// get previous state of word-done
	int i;
	UINT8 s0, s1;

	LOG((LOG_DISK,8,"	>>> KWD timing bitclk:%d datin:%d block:%d SECT[4]:%d\n", bitclk, datin, block, dhd->get_sector_mark_0()));
	if (0 == m_dsk.seclate) {
		// if SECLATE is 0, WDDONE' never goes low (counter's clear has precedence).
		if (m_dsk.bitcount || m_dsk.carry) {
			LOG((LOG_DISK,7,"	SECLATE:0 clears bitcount:0\n"));
			m_dsk.bitcount = 0;
			m_dsk.carry = 0;
		}
	} else if (m_dsk.bitclk && !bitclk) {
		// if SECLATE is 1, the counter will count or be loaded
		if ((m_dsk.shiftin & (1 << 16)) && !GET_KCOM_WFFO(m_dsk.kcom)) {
			/*
			 * If HIORDBIT is 1 at the falling edge of BITCLK, it sets the
			 * JK-FF 67b, and thus takes away the LOAD' assertion from the
			 * counter. It has been loaded with 15, so it counts to 16 on
			 * the next rising edge and makes WDDONE' go to 0.
			 */
			LOG((LOG_DISK,7,"	HIORDBIT:1 sets WFFO:1\n"));
			PUT_KCOM_WFFO(m_dsk.kcom, 1);
			// TODO: show disk indicators
		}
		/*
		 * Falling edge of BITCLK, counting continues as it was preset
		 * with BUS[4] (WFFO) at the last KCOM← load, or as set by a
		 * 1 bit being read in HIORDBIT.
		 */
		if (GET_KCOM_WFFO(m_dsk.kcom)) {
			/*
			 * If BUS[4] (WFFO) was 1, both J and K' of the FF (74109) will
			 * be 1 at the rising edge of LDCOM' (at the end of KCOM←)
			 * and Q will be 1. LOAD' is deassterted: count on clock.
			 */
			m_dsk.bitcount = (m_dsk.bitcount + 1) % 16;
			m_dsk.carry = m_dsk.bitcount == 15;
			LOG((LOG_DISK,6,"	WFFO:1 count bitcount:%2d\n", m_dsk.bitcount));
		} else {
			/*
			 * If BUS[4] (WFFO) was 0, both J and K' will be 0, and Q
			 * will be 0. LOAD' is asserted and will load on rising bitclock (now)
			 */
			m_dsk.bitcount = 15;
			m_dsk.carry = 1;
			LOG((LOG_DISK,6,"	WFFO:0 load bitcount:%2d\n", m_dsk.bitcount));
		}
	} else if (!m_dsk.bitclk && bitclk) {
		// rising edge of bitclk
		m_dsk.shiftin = (m_dsk.shiftin << 1) | datin;		// clock the input shift register
		m_dsk.shiftout = m_dsk.shiftout << 1;				// and the output shift register too
	}

	if (m_dsk.wddone != wddone) {
		LOG((LOG_DISK,8,"	WDDONE':%d→%d\n", m_dsk.wddone, wddone));
	}

	if (m_dsk.carry) {
		/* CARRY = 1 -> WDDONE' = 0 */
		wddone = 0;
		if (m_dsk.wddone == 0) {
			/*
			 * Latch a new data word while WDDONE is 0
			 * Note: The shifter outputs for bits 0 to 14 are connected
			 * to the latches inputs 1 to 15, while input bit 0 comes
			 * from the current datin.
			 * Shifter output 15 is the HIORDBIT signal.
			 */
			m_dsk.datain = m_dsk.shiftin & 0177777;
			/* load the output shift register */
			m_dsk.shiftout = m_dsk.dataout;
			LOG((LOG_DISK,8," 	LATCH in:%06o (0x%04x) out:%06o (0x%04x)\n", m_dsk.datain, m_dsk.datain, m_dsk.dataout, m_dsk.dataout));
		}
	} else {
		/* CARRY = 0 -> WDDONE' = 1 */
		wddone = 1;
	}

	// remember previous state of word-done
	m_dsk.wddone = wddone;

	/**
	 * JK flip-flop 43b (word task)
	 * <PRE>
	 * CLK	WDDONE'
	 * J	1
	 * K'	1
	 * S'	1
	 * C'	WDTSKENA
	 * Q	to 53a J
	 * </PRE>
	 */
	DEBUG_NAME("\t\t43b KWD   ");
	s0 = m_dsk.ff_43b;
	s1 = wddone ? JKFF_CLK : 0;
	s1 |= JKFF_J;
	s1 |= JKFF_K;
	if (m_dsk.ok_to_run)
		s1 |= JKFF_S;
	if (!(m_dsk.ff_43a & JKFF_Q))
		s1 |= JKFF_C;
	m_dsk.ff_43b = update_jkff(s0, s1);

	// loop over the 4 stages of sysclka and sysclkb transitions
	for (i = 0; i < 4; i++) {

		if (m_sysclka0[i] != m_sysclka1[i]) {
			LOG((LOG_DISK,9,"	SYSCLKA' %s\n", raise_lower[m_sysclka1[i]]));
		}
		if (m_sysclkb0[i] != m_sysclkb1[i]) {
			LOG((LOG_DISK,9,"	SYSCLKB' %s\n", raise_lower[m_sysclkb1[i]]));
		}

		/**
		 * JK flip-flop 53b (word task)
		 * <PRE>
		 * CLK	SYSCLKB'
		 * J	0
		 * K'	(BLOCK & WDTSKACT)'
		 * S'	WDALLOW
		 * C'	1
		 * Q	WDINIT
		 * </PRE>
		 */
		DEBUG_NAME("\t\t53b KWD   ");
		s0 = m_dsk.ff_53b;
		s1 = m_sysclkb1[i] ? JKFF_CLK : JKFF_0;
		if (block != task_kwd)
			s1 |= JKFF_K;			// (BLOCK & WDTSKACT)'
		if (WDALLOW)
			s1 |= JKFF_S;
		s1 |= JKFF_C;
		m_dsk.ff_53b = update_jkff(s0, s1);

		/**
		 * JK flip-flop 53a (word task)
		 * <PRE>
		 * CLK	SYSCLKB'
		 * J	from 43b Q
		 * K'	(BLOCK & WDTSKACT)'
		 * S'	1
		 * C'	WDALLOW
		 * Q	to 43a J and K'
		 * </PRE>
		 */
		DEBUG_NAME("\t\t53a KWD   ");
		s0 = m_dsk.ff_53a;
		s1 = m_sysclkb1[i] ? JKFF_CLK : JKFF_0;
		if (m_dsk.ff_43b & JKFF_Q)
			s1 |= JKFF_J;
		if (block != task_kwd)
			s1 |= JKFF_K;
		s1 |= JKFF_S;
		if (WDALLOW)
			s1 |= JKFF_C;
		m_dsk.ff_53a = update_jkff(s0, s1);

		/**
		 * JK flip-flop 43a (word task)
		 * <PRE>
		 * CLK	SYSCLKA'
		 * J	from 53a Q
		 * K'	from 53a Q
		 * S'	1
		 * C'	WDALLOW
		 * Q	WDTSKENA', Q' WDTSKENA
		 * </PRE>
		 */
		DEBUG_NAME("\t\t43a KWD   ");
		s0 = m_dsk.ff_43a;
		s1 = m_sysclka1[i] ? JKFF_CLK : JKFF_0;
		if (m_dsk.ff_53a & JKFF_Q)
			s1 |= JKFF_J;
		if (m_dsk.ff_53a & JKFF_Q)
			s1 |= JKFF_K;
		s1 |= JKFF_S;
		if (WDALLOW)
			s1 |= JKFF_C;
		m_dsk.ff_43a = update_jkff(s0, s1);

		/**
		 * JK flip-flop 45a (ready latch)
		 * <PRE>
		 * CLK	SYSCLKA'
		 * J	READY' from drive
		 * K'	1
		 * S'	1
		 * C'	CLRSTAT'
		 * Q	RDYLAT'
		 * </PRE>
		 */
		DEBUG_NAME("\t\t45a RDYLAT");
		s0 = m_dsk.ff_45a;
		s1 = m_sysclka1[i] ? JKFF_CLK : JKFF_0;
		if (dhd->get_ready_0())
			s1 |= JKFF_J;
		s1 |= JKFF_K;
		s1 |= JKFF_S;
		s1 |= JKFF_C;		// FIXME: CLRSTAT' ?
		m_dsk.ff_45a = update_jkff(s0, s1);

		/**
		 * sets the seqerr flip-flop 45b (Q' is SEQERR)
		 * JK flip-flop 45b (seqerr latch)
		 * <PRE>
		 * CLK	SYSCLKA'
		 * J	1
		 * K'	SEQERR'
		 * S'	CLRSTAT'
		 * C'	1
		 * Q	to KSTAT[11] DATALATE
		 * </PRE>
		 */
		DEBUG_NAME("\t\t45b SEQERR");
		s0 = m_dsk.ff_45b;
		s1 = m_sysclka1[i] ? JKFF_CLK : JKFF_0;
		s1 |= JKFF_J;
		if (SEQERR)
			s1 |= JKFF_K;
		s1 |= JKFF_S;		// FIXME: CLRSTAT' ?
		s1 |= JKFF_C;
		m_dsk.ff_45b = update_jkff(s0, s1);

		/**
		 * JK flip-flop 22b (sector task)
		 * <PRE>
		 * CLK	SYSCLKB'
		 * J	from 22a Q
		 * K'	(BLOCK & STSKACT)'
		 * S'	1 (really it's RESET')
		 * C'	1
		 * Q	STSKENA; Q' WAKEKST'
		 * </PRE>
		 */
		DEBUG_NAME("\t\t22b KSEC  ");
		s0 = m_dsk.ff_22b;
		s1 = m_sysclkb1[i] ? JKFF_CLK : JKFF_0;
		if (m_dsk.ff_22a & JKFF_Q)
			s1 |= JKFF_J;
		if (block != task_ksec)
			s1 |= JKFF_K;
		s1 |= JKFF_S;	// FIXME: RESET' ?
		s1 |= JKFF_C;
		m_dsk.ff_22b = update_jkff(s0, s1);

		/**
		 * JK flip-flop 22a (sector task)
		 * <PRE>
		 * CLK	SYSCLKB'
		 * J	from 21b Q
		 * K'	1
		 * S'	1
		 * C'	WAKEST'
		 * Q	to 22b J
		 * </PRE>
		 */
		DEBUG_NAME("\t\t22a KSEC  ");
		s0 = m_dsk.ff_22a;
		s1 = m_sysclkb1[i] ? JKFF_CLK : JKFF_0;
		if (m_dsk.ff_21b & JKFF_Q)
			s1 |= JKFF_J;
		s1 |= JKFF_K;
		s1 |= JKFF_S;
		if (!(m_dsk.ff_22b & JKFF_Q))
			s1 |= JKFF_C;
		m_dsk.ff_22a = update_jkff(s0, s1);

		/**
		 * JK flip-flop 21b (sector task)
		 * <PRE>
		 * CLK	SYSCLKB'
		 * J	from 21a Q
		 * K'	1
		 * S'	1
		 * C'	WAKEST'
		 * Q	to 22a J
		 * </PRE>
		 */
		DEBUG_NAME("\t\t21b KSEC  ");
		s0 = m_dsk.ff_21b;
		s1 = m_sysclkb1[i] ? JKFF_CLK : JKFF_0;
		if (m_dsk.ff_21a & JKFF_Q)
			s1 |= JKFF_J;
		s1 |= JKFF_K;
		s1 |= JKFF_S;
		if (!(m_dsk.ff_22b & JKFF_Q))
			s1 |= JKFF_C;
		m_dsk.ff_21b = update_jkff(s0, s1);
	}

	// The 53b FF Q output is the WDINIT signal.
	if (WDINIT != m_dsk.wdinit) {
		m_dsk.wdinit0 = m_dsk.wdinit;
		// rising edge immediately
		if ((m_dsk.wdinit = WDINIT) == 1)
			m_dsk.wdinit0 = 1;
		LOG((LOG_DISK,8,"	WDINIT:%d\n", m_dsk.wdinit));
	}

	/*
	 * If Q (53a) and Q (43a) are both 1, the WAKEKWDT'
	 * output is 0 and the disk word task wakeup signal is asserted.
	 */
	if (m_dsk.ff_53a & m_dsk.ff_43a & JKFF_Q) {
		if (m_dsk.wdtskena == 1) {
			LOG((LOG_DISK,2,"	WDTSKENA':0 and WAKEKWDT':0 wake KWD\n"));
			m_dsk.wdtskena = 0;
			m_task_wakeup |= 1 << task_kwd;
		}
	} else if (m_dsk.ff_43a & JKFF_Q) {
		/*
		 * If Q (43a) is 1, the WDTSKENA' signal is deasserted.
		 */
		if (m_dsk.wdtskena == 0) {
			LOG((LOG_DISK,2,"	WDTSKENA':1\n"));
			m_dsk.wdtskena = 1;
			m_task_wakeup &= ~(1 << task_kwd);
		}
	}

	if (0 != m_dsk.kfer) {
		// no fatal error: ready AND not seqerr AND seekok
		if (!RDYLAT && !SEQERR && SEEKOK) {
			LOG((LOG_DISK,6,"	reset KFER\n"));
			m_dsk.kfer = 0;
		}
	} else {
		// fatal error: not ready OR seqerr OR not seekok
		if (RDYLAT) {
			LOG((LOG_DISK,6,"	RDYLAT sets KFER\n"));
			m_dsk.kfer = 1;
		}
		if (SEQERR) {
			LOG((LOG_DISK,6,"	SEQERR sets KFER\n"));
			m_dsk.kfer = 1;
		}
		if (!SEEKOK) {
			LOG((LOG_DISK,6,"	not SEEKOK sets KFER\n"));
			m_dsk.kfer = 1;
		}
	}

	/*
	 * The FF 22b Q output is the STSKENA (sector task enable)
	 * signal, the Q' is the WAKEKST' signal.
	 */
	if (m_dsk.ff_22b & JKFF_Q) {
		if (0 == (m_task_wakeup & (1 << task_ksec))) {
			LOG((LOG_DISK,6,"	STSKENA:1; WAKEST':0 wake KSEC\n"));
			m_task_wakeup |= 1 << task_ksec;
		}
	} else {
		if (0 != (m_task_wakeup & (1 << task_ksec))) {
			LOG((LOG_DISK,6,"	STSKENA:0; WAKEST':1\n"));
			m_task_wakeup &= ~(1 << task_ksec);
		}
	}

	/**
	 * JK flip-flop 21a (sector task)
	 * <PRE>
	 * CLK	SECT4 (inverted sector mark from drive)
	 * J	WAKEST'
	 * K'	1
	 * S'	ERRWAKE'
	 * C'	WAKEST'
	 * Q	to seclate monoflop
	 * </PRE>
	 */
	DEBUG_NAME("\t\t21a KSEC  ");
	s0 = m_dsk.ff_21a;
	s1 = dhd->get_sector_mark_0() ? JKFF_0 : JKFF_CLK;
	if (!(m_dsk.ff_22b & JKFF_Q))
		s1 |= JKFF_J;
	s1 |= JKFF_K;
	if (!ERRWAKE)
		s1 |= JKFF_S;
	if (!(m_dsk.ff_22b & JKFF_Q))
		s1 |= JKFF_C;
	m_dsk.ff_21a = update_jkff(s0, s1);

	// If the KSEC FF 21a Q goes 1, pulse the SECLATE signal for some time.
	if ((m_dsk.ff_21a_old & JKFF_Q0) && (m_dsk.ff_21a & JKFF_Q)) {
		m_dsk.seclate_timer->adjust(attotime::from_nsec(TW_SECLATE), 1);
		if (m_dsk.seclate) {
			m_dsk.seclate = 0;
			LOG((LOG_DISK,6,"	SECLATE -> 0 pulse until %lldns\n", ntime() + TW_SECLATE));
		}
	}

	// check if write and erase gate, or read gate are changed
	if ((m_task_wakeup & (1 << task_ksec)) || GET_KCOM_XFEROFF(m_dsk.kcom) || m_dsk.kfer) {
#if	ALTO2_DEBUG
		if (0 == m_dsk.egate || 0 == m_dsk.wrgate || 0 == m_dsk.rdgate) {
			// log the reason why gates are deasserted
			LOG((LOG_DISK,6,"	deassert gates because of"));
			if (m_task_wakeup & (1 << task_ksec)) {
				LOG((LOG_DISK,6," wake KSEC"));
			}
			if (GET_KCOM_XFEROFF(m_dsk.kcom)) {
				LOG((LOG_DISK,6," XFEROFF"));
			}
			if (m_dsk.kfer) {
				LOG((LOG_DISK,6," KFER"));
			}
		}
#endif
		// sector task is active OR xferoff is set OR fatal error
		dhd->set_egate(m_dsk.egate = 1);
		dhd->set_wrgate(m_dsk.wrgate = 1);
		dhd->set_rdgate(m_dsk.rdgate = 1);
	} else {
		if (m_dsk.krwc & RWC_WRITE) {
			if (m_dsk.ok_to_run) {
#if	ALTO2_DEBUG
				if (1 == m_dsk.egate || 1 == m_dsk.wrgate) {
					LOG((LOG_DISK,6,"	assert "));
					if (m_dsk.egate) {
						LOG((LOG_DISK,6," EGATE"));
					}
					if (m_dsk.wrgate) {
						LOG((LOG_DISK,6," WRGATE"));
					}
				}
#endif
				// assert erase and write gates
				dhd->set_egate(m_dsk.egate = 0);
				dhd->set_wrgate(m_dsk.wrgate = 0);
			}
		} else {
#if	ALTO2_DEBUG
			if (1 == m_dsk.rdgate) {
				LOG((LOG_DISK,6,"	assert RDGATE"));
			}
#endif
			// assert read gate
			dhd->set_rdgate(m_dsk.rdgate = 0);
		}
	}

	m_dsk.ff_21a_old = m_dsk.ff_21a;
	m_dsk.bitclk = bitclk;
	m_dsk.datin = datin;
	LOG((LOG_DISK,8,"	<<< KWD timing\n"));
}


/** @brief timer callback to take away the SECLATE pulse (monoflop) */
void alto2_cpu_device::disk_seclate(void* ptr, INT32 arg)
{
	(void)ptr;
	LOG((LOG_DISK,2,"	SECLATE -> %d\n", arg));
	m_dsk.seclate = arg;
	m_dsk.seclate_timer->enable(false);
}

/** @brief timer callback to take away the OK TO RUN pulse (reset) */
void alto2_cpu_device::disk_ok_to_run(void* ptr, INT32 arg)
{
	(void)ptr;
	LOG((LOG_DISK,2,"	OK TO RUN -> %d\n", arg));
	m_dsk.ok_to_run = arg;
	m_dsk.ok_to_run_timer->reset();

	for (int unit = 0; unit < diablo_hd_device::DIABLO_UNIT_MAX; unit++) {
		diablo_hd_device* dhd = m_drive[unit];
		dhd->set_sector_callback(this, &disk_sector_start);
	}
}

/**
 * @brief timer callback to pulse the STROBE' signal to the drive
 *
 * STROBE' pulses are sent to the drive at a rate that depends on
 * the monoflop 52b external resistor and capacitor.
 *
 * The drive compares the cylinder number that is presented on
 * it's inputs against the current cylinder, and if they don't
 * match steps into the corresponding direction.
 *
 * On the falling edge of a strobe, the drive sets the log_addx_interlock
 * flag 0 (LAI, active low). On the rising edge of the strobe the drive then
 * indicates seek completion by setting addx_acknowledge to 0 (ADDRACK, active low).
 * If the seek is not yet complete, it instead keeps the seek_incomplete
 * flag 0 (SKINC, active low). If the seek would go beyond the last cylinder,
 * the drive deasserts seek_incomplete, but does not assert the addx_acknowledge.
 *
 * @param id timer id
 * @param arg contains the drive, cylinder, and restore flag
 */
void alto2_cpu_device::disk_strobon(void* ptr, INT32 arg)
{
	(void)ptr;
	UINT8 unit = static_cast<UINT8>(arg & 1);
	UINT8 restore = static_cast<UINT8>((arg >> 1) & 1);
	INT32 cylinder = arg >> 2;
	int seekok;
	int lai;
	int strobe;

	diablo_hd_device* dhd = m_drive[unit];
	if (!dhd) {
		// FIXME: set all signals for a not connected drive
		return;
	}
	LOG((LOG_DISK,2,"	STROBE #%d restore:%d cylinder:%d\n", unit, restore, cylinder));

	/* This is really monoflop 52a generating a very short 0 pulse */
	for (strobe = 0; strobe < 2; strobe++) {
		UINT8 s0, s1;
		/* pulse the strobe signal to the unit */
		dhd->set_strobe(cylinder, restore, strobe);

		lai = dhd->get_log_addx_interlock_0();
		LOG((LOG_DISK,6,"		LAI':%d\n", lai));
		/**
		 * JK flip-flop 44a (LAI' clocked)
		 * <PRE>
		 * CLK	LAI
		 * J	1
		 * K'	1
		 * S'	1
		 * C'	CLRSTAT' (not now)
		 * Q	to seekok
		 * </PRE>
		 */
		DEBUG_NAME("\t\t44a LAI   ");
		s0 = m_dsk.ff_44a;
		s1 = lai ? JKFF_CLK : JKFF_0;
		s1 |= JKFF_J;
		s1 |= JKFF_K;
		s1 |= JKFF_S;
		s1 |= JKFF_C;
		m_dsk.ff_44a = update_jkff(s0, s1);
		if (dhd->get_addx_acknowledge_0() == 0 && (m_dsk.ff_44a & JKFF_Q)) {
			/* if address is acknowledged, and Q' of FF 44a, clear the strobe */
			m_dsk.strobe = 0;
		}
	}

	if (dhd->get_addx_acknowledge_0()) {

		/* no acknowledge yet */

	} else {
		/* clear the monoflop 52b, i.e. no timer restart */
		LOG((LOG_DISK,2,"		STROBON:%d\n", m_dsk.strobe));

		/* update the seekok status: SKINC' && LAI' && Q' of FF 44a */
		seekok = dhd->get_seek_incomplete_0();
		if (seekok != m_dsk.seekok) {
			m_dsk.seekok = seekok;
			LOG((LOG_DISK,2,"		SEEKOK:%d\n", m_dsk.seekok));
		}
	}

	LOG((LOG_DISK,2,"	current cylinder:%d\n", dhd->get_cylinder()));

	/* if the strobe is still set, restart the timer */
	if (m_dsk.strobe) {
		m_dsk.strobon_timer->adjust(attotime::from_nsec(TW_STROBON), arg);
	} else {
		m_dsk.strobon_timer->reset();
	}
}

/** @brief timer callback to change the READY monoflop 31a */
void alto2_cpu_device::disk_ready_mf31a(void* ptr, INT32 arg)
{
	diablo_hd_device* dhd = m_drive[m_dsk.drive];
	m_dsk.ready_mf31a = arg & dhd->get_ready_0();
	/* log the not ready result with level 0, else 2 */
	LOG((LOG_DISK,m_dsk.ready_mf31a ? 0 : 2,"	ready mf31a:%d\n", m_dsk.ready_mf31a));
}

/**
 * @brief called if one of the disk tasks (task_kwd or task_ksec) blocks
 *
 * @param task task that blocks (either task_ksec or task_kwd)
 */
void alto2_cpu_device::disk_block(int task)
{
	kwd_timing(m_dsk.bitclk, m_dsk.datin, task);
}

/**
 * @brief bs_read_kstat early: bus driven by disk status register KSTAT
 * <PRE>
 * Part of the KSTAT register is made of two 4 bit latches S8T10 (Signetics).
 * The signals BUS[8-11] are the current state of:
 *     BUS[0-3]   SECT[0-3]; from the Winchester drive (inverted)
 *     BUS[8]     SEEKOK'
 *     BUS[9]     SRWRDY' from the Winchester drive
 *     BUS[10]    RDYLAT' (latched READY' at last CLRSTAT, FF 45a output Q)
 *     BUS[11]    SEQERR (latched SEQERR at last CLRSTAT, FF 45b output Q')
 * The signals BUS[12,14-15] are just as they were loaded at KSTAT← time.
 *     BUS[13]    CHSEMERROR (FF 44b output Q' inverted)
 * </PRE>
 */
void alto2_cpu_device::bs_read_kstat_0()
{
	UINT16 r;
	int unit = m_dsk.drive;
	diablo_hd_device* dhd = m_drive[unit];

	/* KSTAT[4-7] bus is open */
	PUT_KSTAT_DONE(m_dsk.kstat, 017);

	/* KSTAT[8] latch the inverted seekok status */
	PUT_KSTAT_SEEKFAIL(m_dsk.kstat, m_dsk.seekok ? 0 : 1);

	/* KSTAT[9] latch the drive seek/read/write status */
	PUT_KSTAT_SEEK(m_dsk.kstat, dhd->get_seek_read_write_0());

	/* KSTAT[10] latch the latched (FF 45a at CLRSTAT) ready status */
	PUT_KSTAT_NOTRDY(m_dsk.kstat, m_dsk.ff_45a & JKFF_Q ? 1 : 0);

	/* KSTAT[11] latch the latched (FF 45b at CLRSTAT) seqerr status (Q') */
	PUT_KSTAT_DATALATE(m_dsk.kstat, m_dsk.ff_45b & JKFF_Q ? 0 : 1);

	/* KSTAT[13] latch the latched (FF 44b at CLRSTAT/KSTAT←) checksum status */
	PUT_KSTAT_CKSUM(m_dsk.kstat, m_dsk.ff_44b & JKFF_Q ? 1 : 0);

	r = m_dsk.kstat;

	LOG((LOG_DISK,1,"	←KSTAT; BUS &= %#o\n", r));
	LOG((LOG_DISK,2,"		SECTOR     : %#o\n", GET_KSTAT_SECTOR(m_dsk.kstat)));
	LOG((LOG_DISK,2,"		DONE       : %#o\n", GET_KSTAT_DONE(m_dsk.kstat)));
	LOG((LOG_DISK,2,"		SEEKFAIL   : %d\n", GET_KSTAT_SEEKFAIL(m_dsk.kstat)));
	LOG((LOG_DISK,2,"		SEEK       : %d\n", GET_KSTAT_SEEK(m_dsk.kstat)));
	LOG((LOG_DISK,2,"		NOTRDY     : %d\n", GET_KSTAT_NOTRDY(m_dsk.kstat)));
	LOG((LOG_DISK,2,"		DATALATE   : %d\n", GET_KSTAT_DATALATE(m_dsk.kstat)));
	LOG((LOG_DISK,2,"		IDLE       : %d\n", GET_KSTAT_IDLE(m_dsk.kstat)));
	LOG((LOG_DISK,2,"		CKSUM      : %d\n", GET_KSTAT_CKSUM(m_dsk.kstat)));
	LOG((LOG_DISK,2,"		COMPLETION : %#o\n", GET_KSTAT_COMPLETION(m_dsk.kstat)));

	m_bus &= r;
}

/**
 * @brief bs_read_kdata early: bus driven by disk data register KDATA input
 *
 * The input data register is a latch that latches the contents of
 * the lower 15 bits of a 16 bit shift register in its more significant
 * 15 bits, and the current read data bit is the least significant
 * bit. This is handled in kwd_timing.
 */
void alto2_cpu_device::bs_read_kdata_0()
{
	UINT16 r;
	/* get the current word from the drive */
	r = m_dsk.datain;
	LOG((LOG_DISK,1,"	←KDATA (%#o)\n", r));
	m_bus &= r;
}

/**
 * @brief f1_strobe late: initiates a disk seek
 *
 * Initiates a disk seek operation. The KDATA register must have
 * been loaded previously, and the SENDADR bit of the KCOM
 * register previously set to 1.
 */
void alto2_cpu_device::f1_strobe_1()
{
	if (GET_KCOM_SENDADR(m_dsk.kcom)) {
		LOG((LOG_DISK,1,"	STROBE (SENDADR:1)\n"));
		/* Set the STROBON flag and start the STROBON monoflop */
		m_dsk.strobe = 1;
		disk_strobon(0,
			4 * GET_KADDR_CYLINDER(m_dsk.kaddr) +
			2 * GET_KADDR_RESTORE(m_dsk.kaddr) +
			m_dsk.drive);
	} else {
		LOG((LOG_DISK,1,"	STROBE (w/o SENDADR)\n"));
		/* FIXME: what to do if SENDADR isn't set? */
	}
}

/**
 * @brief f1_load_kstat late: load disk status register
 *
 * KSTAT[12-15] are loaded from BUS[12-15], except that BUS[13] is
 * ORed into KSTAT[13].
 *
 * NB: The 4 bits are just software, not changed by hardware
 */
void alto2_cpu_device::f1_load_kstat_1()
{
	int i;
	LOG((LOG_DISK,1,"	KSTAT←; BUS[12-15] %#o\n", m_bus));
	LOG((LOG_DISK,2,"		IDLE       : %d\n", GET_KSTAT_IDLE(m_bus)));
	LOG((LOG_DISK,2,"		CKSUM      : %d\n", GET_KSTAT_CKSUM(m_bus)));
	LOG((LOG_DISK,2,"		COMPLETION : %#o\n", GET_KSTAT_COMPLETION(m_bus)));

	/* KSTAT[12] is just taken from BUS[12] */
	PUT_KSTAT_IDLE(m_dsk.kstat, GET_KSTAT_IDLE(m_bus));

	/* KSTAT[14-15] are just taken from BUS[14-15] */
	PUT_KSTAT_COMPLETION(m_dsk.kstat, GET_KSTAT_COMPLETION(m_bus));

	/* May set the the CKSUM flip-flop 44b
	 * JK flip-flop 44b (KSTAT← clocked)
	 * CLK	SYSCLKA'
	 * J	!BUS[13]
	 * K'	1
	 * S'	1
	 * C'	CLRSTAT' (not now)
	 * Q	Q' inverted to BUS[13] on ←KSTAT
	 */
	for (i = 0; i < 2; i++) {
		UINT8 s0, s1;
		DEBUG_NAME("\t\t44b CKSUM ");
		s0 = m_dsk.ff_44b;
		s1 = i ? JKFF_CLK : 0;
		if (!A2_GET16(m_bus,16,13,13))
			s1 |= JKFF_J;
		s1 |= JKFF_K;
		s1 |= JKFF_S;
		s1 |= JKFF_C;
		m_dsk.ff_44b = update_jkff(s0, s1);
	}
}

/**
 * @brief f1_load_kdata late: load data out register, or the disk address register
 *
 * KDATA is loaded from BUS.
 */
void alto2_cpu_device::f1_load_kdata_1()
{
	m_dsk.dataout = m_bus;
	if (GET_KCOM_SENDADR(m_dsk.kcom)) {
		PUT_KADDR_SECTOR(m_dsk.kaddr, GET_KADDR_SECTOR(m_bus));
		PUT_KADDR_CYLINDER(m_dsk.kaddr, GET_KADDR_CYLINDER(m_bus));
		PUT_KADDR_HEAD(m_dsk.kaddr, GET_KADDR_HEAD(m_bus));
		PUT_KADDR_DRIVE(m_dsk.kaddr, GET_KADDR_DRIVE(m_bus));
		PUT_KADDR_RESTORE(m_dsk.kaddr, GET_KADDR_RESTORE(m_bus));
		PUT_KADDR_DRIVE(m_dsk.kaddr, GET_KADDR_DRIVE(m_bus));
		m_dsk.drive = GET_KADDR_DRIVE(m_dsk.kaddr);

		LOG((LOG_DISK,1,"	KDATA←; BUS (%#o) (drive:%d restore:%d %d/%d/%02d)\n",
			m_bus,
			GET_KADDR_DRIVE(m_dsk.kaddr),
			GET_KADDR_RESTORE(m_dsk.kaddr),
			GET_KADDR_CYLINDER(m_dsk.kaddr),
			GET_KADDR_HEAD(m_dsk.kaddr),
			GET_KADDR_SECTOR(m_dsk.kaddr)));
#if	0
		/* printing changes in the disk address */
		{
			static int last_kaddr;
			if (m_dsk.kaddr != last_kaddr) {
				int c = GET_KADDR_CYLINDER(m_dsk.kaddr);
				int h = GET_KADDR_HEAD(m_dsk.kaddr);
				int s = GET_KADDR_SECTOR(m_dsk.kaddr);
				int page = DRIVE_PAGE(c,h,s);
				last_kaddr = m_dsk.kaddr;
				printf("	unit:%d restore:%d %d/%d/%02d page:%d\n",
					GET_KADDR_DRIVE(m_dsk.kaddr),
					GET_KADDR_RESTORE(m_dsk.kaddr),
					c, h, s, page);
			}
		}
#endif
	} else {
		LOG((LOG_DISK,1,"	KDATA←; BUS %#o (%#x)\n", m_bus, m_bus));
	}
}

/**
 * @brief f1_increcno late: advances shift registers holding KADR
 *
 * Advances the shift registers holding the KADR register so that they
 * present the number and read/write/check status of the next record
 * to the hardware.
 *
 * <PRE>
 * Sheet 10, shifter (74195) parts #36 and #37
 *
 * Vcc, BUS[08], BUS[10], BUS[12] go to #36 A,B,C,D
 * Vcc, BUS[09], BUS[11], BUS[13] go to #37 A,B,C,D
 * A is connected to ground on both chips;
 * both shifters are loaded with KADR←
 *
 * The QA outputs are #36 -> RECNO(0) and #37 -> RECNO(1)
 *
 * RECNO(0) (QA of #37) goes to J and K' of #36
 * RECNO(1) (QA of #36) is inverted and goes to J and K' of #37
 *
 *  shift/   RECNO(0)    RECNO(1)     R/W/C presented
 *   load      #37         #36        to the drive
 * ---------------------------------------------------
 *   load       0           0         HEADER
 * 1st shift    1           0         LABEL
 * 2nd shift    1           1         DATA
 * 3rd shift    0           1         (none) 0 = read
 * [ 4th        0           0         (none) 2 = write ]
 * [ 5th        1           0         (none) 3 = write ]
 * [ 6th        1           1         (none) 1 = check ]
 * </PRE>
 */
void alto2_cpu_device::f1_increcno_1()
{
	switch (m_dsk.krecno) {
	case RECNO_HEADER:
		m_dsk.krecno = RECNO_LABEL;
		m_dsk.krwc = GET_KADR_LABEL(m_dsk.kadr);
		LOG((LOG_DISK,2,"	INCRECNO; HEADER → LABEL (%o, rwc:%o)\n", m_dsk.krecno, m_dsk.krwc));
		break;
	case RECNO_PAGENO:
		m_dsk.krecno = RECNO_HEADER;
		m_dsk.krwc = GET_KADR_HEADER(m_dsk.kadr);
		LOG((LOG_DISK,2,"	INCRECNO; PAGENO → HEADER (%o, rwc:%o)\n", m_dsk.krecno, m_dsk.krwc));
		break;
	case RECNO_LABEL:
		m_dsk.krecno = RECNO_DATA;
		m_dsk.krwc = GET_KADR_DATA(m_dsk.kadr);
		LOG((LOG_DISK,2,"	INCRECNO; LABEL → DATA (%o, rwc:%o)\n", m_dsk.krecno, m_dsk.krwc));
		break;
	case RECNO_DATA:
		m_dsk.krecno = RECNO_PAGENO;
		m_dsk.krwc = 0;	/* read (?) */
		LOG((LOG_DISK,2,"	INCRECNO; DATA → PAGENO (%o, rwc:%o)\n", m_dsk.krecno, m_dsk.krwc));
		break;
	}
	// TODO: show disk indicator
}

/**
 * @brief f1_clrstat late: reset all error latches
 *
 * Causes all error latches in the disk controller hardware to reset,
 * clears KSTAT[13].
 *
 * NB: IDLE (KSTAT[12]) and COMPLETION (KSTAT[14-15]) are not cleared
 */
void alto2_cpu_device::f1_clrstat_1()
{
	diablo_hd_device* dhd = m_drive[m_dsk.drive];
	LOG((LOG_DISK,1,"	CLRSTAT\n"));
	UINT8 s0, s1;

	/* clears the LAI clocked flip-flop 44a
	 * JK flip-flop 44a (LAI' clocked)
	 * CLK	(LAI')'
	 * J	1
	 * K'	1
	 * S'	1
	 * C'	CLRSTAT'
	 * Q	to seekok
	 */
	DEBUG_NAME("\t\t44a LAI   ");
	s0 = m_dsk.ff_44a;
	s1 = m_dsk.ff_44a & JKFF_CLK;
	s1 |= JKFF_J;
	s1 |= JKFF_K;
	s1 |= JKFF_S;
	m_dsk.ff_44a = update_jkff(s0, s1);

	/* clears the CKSUM flip-flop 44b
	 * JK flip-flop 44b (KSTAT← clocked)
	 * CLK	SYSCLKA' (not used here, just clearing)
	 * J	1 (BUS[13] during KSTAT←)
	 * K'	1
	 * S'	1
	 * C'	CLRSTAT'
	 * Q	to seekok
	 */
	DEBUG_NAME("\t\t44b CKSUM ");
	s0 = m_dsk.ff_44b;
	s1 = m_dsk.ff_44b & JKFF_CLK;
	s1 |= m_dsk.ff_44b & JKFF_J;
	s1 |= JKFF_K;
	s1 |= JKFF_S;
	m_dsk.ff_44b = update_jkff(s0, s1);

	/* clears the rdylat flip-flop 45a
	 * JK flip-flop 45a (ready latch)
	 * CLK	SYSCLKA'
	 * J	READY' from drive
	 * K'	1
	 * S'	1
	 * C'	CLRSTAT'
	 * Q	RDYLAT'
	 */
	DEBUG_NAME("\t\t45a RDYLAT");
	s0 = m_dsk.ff_45a;
	s1 = m_dsk.ff_45a & JKFF_CLK;
	if (dhd)
		s1 |= dhd->get_ready_0() ? JKFF_J : 0;
	else
		s1 |= JKFF_J;
	s1 |= JKFF_K;
	s1 |= JKFF_S;
	m_dsk.ff_45a = update_jkff(s0, s1);

	/* sets the seqerr flip-flop 45b (Q' is SEQERR)
	 * JK flip-flop 45b (seqerr latch)
	 * CLK	SYSCLKA'
	 * J	1
	 * K'	SEQERR'
	 * S'	CLRSTAT'
	 * C'	1
	 * Q	to KSTAT[11] DATALATE
	 */
	DEBUG_NAME("\t\t45b SEQERR");
	s0 = m_dsk.ff_45b;
	s1 = m_dsk.ff_45b & JKFF_CLK;
	s1 |= JKFF_J;
	if (!SEQERR)
		s1 |= JKFF_K;
	s1 |= JKFF_C;
	m_dsk.ff_45b = update_jkff(s0, s1);

	/* set or reset monoflop 31a, depending on drive READY' */
	m_dsk.ready_mf31a = dhd->get_ready_0();

	/* start monoflop 31a, which resets ready_mf31a */
	m_dsk.ready_timer->adjust(attotime::from_nsec(TW_READY), 1);
}

/**
 * @brief f1_load_kcom late: load the KCOM register from bus
 * <PRE>
 * This causes the KCOM register to be loaded from BUS[1-5]. The
 * KCOM register has the following interpretation:
 *	(1) XFEROFF = 1 inhibits data transmission to/from the m_dsk.
 *	(2) WDINHIB = 1 prevents the disk word task from awakening.
 * 	(3) BCLKSRC = 0 takes bit clock from disk input or crystal clock, as appropriate.
 *      BCLKSRC = 1 force use of crystal clock.
 *	(4) WFFO = 0 holds the disk bit counter at -1 until a 1 bit is read.
 *	    WFFO = 1 allows the bit counter to proceed normally.
 *	(5) SENDADR = 1 causes KDATA[4-12] and KDATA[15] to be signalled to disk unit as track address.
 *      SENDADR = 0 inhibits such signalling.
 * </PRE>
 */
void alto2_cpu_device::f1_load_kcom_1()
{
	m_dsk.kcom = m_bus;
	LOG((LOG_DISK,2,"	KCOM←; BUS %06o\n", m_dsk.kcom));
	LOG((LOG_DISK,2,"		XFEROFF    : %d\n", GET_KCOM_XFEROFF(m_dsk.kcom)));
	LOG((LOG_DISK,2,"		WDINHIB    : %d\n", GET_KCOM_WDINHIB(m_dsk.kcom)));
	LOG((LOG_DISK,2,"		BCLKSRC    : %d\n", GET_KCOM_BCLKSRC(m_dsk.kcom)));
	LOG((LOG_DISK,2,"		WFFO       : %d\n", GET_KCOM_WFFO(m_dsk.kcom)));
	LOG((LOG_DISK,2,"		SENDADR    : %d\n", GET_KCOM_SENDADR(m_dsk.kcom)));
	// TODO: show disk indicator in the GUI?
}

/**
 * @brief f1_load_kadr late: load the KADR register from bus
 *
 * The KADR register is loaded from BUS[8-14]. This register has the format
 * of word C in section 6.0 above. In addition, it causes the head address
 * bit to be loaded from KDATA[13].
 *
 * NB: the record numer RECNO(0) and RECNO(1) is reset to 0
 */
void alto2_cpu_device::f1_load_kadr_1()
{
	int unit, head;

	/* store into the separate fields of KADR */
	PUT_KADR_SEAL(m_dsk.kadr, GET_KADR_SEAL(m_bus));
	PUT_KADR_HEADER(m_dsk.kadr, GET_KADR_HEADER(m_bus));
	PUT_KADR_LABEL(m_dsk.kadr, GET_KADR_LABEL(m_bus));
	PUT_KADR_DATA(m_dsk.kadr, GET_KADR_DATA(m_bus));
	PUT_KADR_NOXFER(m_dsk.kadr, GET_KADR_NOXFER(m_bus));
	PUT_KADR_UNUSED(m_dsk.kadr, GET_KADR_UNUSED(m_bus));

	unit = GET_KADDR_DRIVE(m_dsk.kaddr);	// get selected drive from DATA[14] output (FF 67a really)
	head = GET_KADDR_HEAD(m_dsk.dataout);	// latch head from DATA[13]
	PUT_KADDR_HEAD(m_dsk.kaddr, head);		// store in KADDR

	/* take the selected head and select drive unit and head in the DIABLO HD */
	diablo_hd_device* dhd = m_drive[unit];
	dhd->select(unit, head);

	/* On KDAR← load bit 0 of parts #36 and #37 is reset to 0, i.e. recno = 0 */
	m_dsk.krecno = 0;
	/* current read/write/check is that for the header */
	m_dsk.krwc = GET_KADR_HEADER(m_dsk.kadr);

	LOG((LOG_DISK,1,"	KADR←; BUS[8-14] #%o\n", m_dsk.kadr));
	LOG((LOG_DISK,2,"		SEAL       : %d\n", GET_KADR_SEAL(m_dsk.kadr)));
	LOG((LOG_DISK,2,"		HEADER     : %s (%#o)\n",  rwc_name[GET_KADR_HEADER(m_dsk.kadr)], GET_KADR_HEADER(m_dsk.kadr)));
	LOG((LOG_DISK,2,"		LABEL      : %s (%#o)\n",  rwc_name[GET_KADR_LABEL(m_dsk.kadr)], GET_KADR_LABEL(m_dsk.kadr)));
	LOG((LOG_DISK,2,"		DATA       : %s (%#o)\n",  rwc_name[GET_KADR_DATA(m_dsk.kadr)], GET_KADR_DATA(m_dsk.kadr)));
	LOG((LOG_DISK,2,"		NOXFER     : %d\n",  GET_KADR_NOXFER(m_dsk.kadr)));
	LOG((LOG_DISK,2,"		unused     : %d (drive?)\n",  GET_KADR_UNUSED(m_dsk.kadr)));
	// TODO: show disk indicator in the GUI?
}

/**
 * @brief f2_init late: branch on disk word task active and init
 *
 * NEXT ← NEXT OR (WDTASKACT && WDINIT ? 037 : 0)
 */
void alto2_cpu_device::f2_init_1()
{
	UINT16 r = INIT ? 037 : 0;
	LOG((LOG_DISK,1,"	INIT; %sbranch (%#o | %#o)\n", r ? "" : "no ", m_next2, r));
	m_next2 |= r;
	m_dsk.wdinit0 = 0;
}

/**
 * @brief f2_rwc late: branch on read/write/check state of the current record
 * <PRE>
 * NEXT ← NEXT OR (current record to be written ? 3 :
 *	current record to be checked ? 2 : 0);
 *
 * NB: note how krecno counts 0,2,3,1 ... etc.
 * on 0: it presents the RWC for HEADER
 * on 2: it presents the RWC for LABEL
 * on 3: it presents the RWC for DATA
 * on 1: it presents the RWC 0, i.e. READ
 *
 * -NEXT[08] = -CHECK = RWC[0] | RWC[1]
 * -NEXT[09] = W/R = RWC[0]
 *
 *  rwc   | -next
 * -------+------
 *  0  0  |  0
 *  0  1  |  2
 *  1  0  |  3
 *  1  1  |  3
 * </PRE>
 */
void alto2_cpu_device::f2_rwc_1()
{
	static UINT16 branch_map[4] = {0,2,3,3};
	UINT16 r = branch_map[m_dsk.krwc];;
	UINT16 init = INIT ? 037 : 0;

	switch (m_dsk.krecno) {
	case RECNO_HEADER:
		LOG((LOG_DISK,1,"	RWC; %sbranch header(%d):%s (%#o|%#o|%#o)\n",
			(r | init) ? "" : "no ", m_dsk.krecno,
			rwc_name[m_dsk.krwc], m_next2, r, init));
		break;
	case RECNO_PAGENO:
		LOG((LOG_DISK,1,"	RWC; %sbranch pageno(%d):%s (%#o|%#o|%#o)\n",
			(r | init) ? "" : "no ", m_dsk.krecno,
			rwc_name[m_dsk.krwc], m_next2, r, init));
		break;
	case RECNO_LABEL:
		LOG((LOG_DISK,1,"	RWC; %sbranch label(%d):%s (%#o|%#o|%#o)\n",
			(r | init) ? "" : "no ", m_dsk.krecno,
			rwc_name[m_dsk.krwc], m_next2, r, init));
		break;
	case RECNO_DATA:
		LOG((LOG_DISK,1,"	RWC; %sbranch data(%d):%s (%#o|%#o|%#o)\n",
			(r | init) ? "" : "no ", m_dsk.krecno,
			rwc_name[m_dsk.krwc], m_next2, r, init));
		break;
	}
	m_next2 |= r | init;
	m_dsk.wdinit0 = 0;
}

/**
 * @brief f2_recno late: branch on the current record number by a lookup table
 * <PRE>
 * NEXT ← NEXT OR MAP (current record number) where
 *   MAP(0) = 0		(header)
 *   MAP(1) = 2		(label)
 *   MAP(2) = 3		(pageno)
 *   MAP(3) = 1		(data)
 * </PRE>
 * NB: The map isn't needed, because m_dsk.krecno counts exactly this way.
 */
void alto2_cpu_device::f2_recno_1()
{
	UINT16 r = m_dsk.krecno;
	UINT16 init = INIT ? 037 : 0;
	LOG((LOG_DISK,1,"	RECNO; %sbranch recno:%d (%#o|%#o|%#o)\n", (r | init) ? "" : "no ", m_dsk.krecno, m_next2, r, init));
	m_next2 |= r | init;
	m_dsk.wdinit0 = 0;
}

/**
 * @brief f2_xfrdat late: branch on the data transfer state
 *
 * NEXT ← NEXT OR (if current command wants data transfer ? 1 : 0)
 */
void alto2_cpu_device::f2_xfrdat_1()
{
	UINT16 r = GET_KADR_NOXFER(m_dsk.kadr) ? 0 : 1;
	UINT16 init = INIT ? 037 : 0;
	LOG((LOG_DISK,1,"	XFRDAT; %sbranch (%#o|%#o|%#o)\n", (r | init) ? "" : "no ", m_next2, r, init));
	m_next2 |= r | init;
	m_dsk.wdinit0 = 0;
}

/**
 * @brief f2_swrnrdy late: branch on the disk ready signal
 *
 * NEXT ← NEXT OR (if disk not ready to accept command ? 1 : 0)
 */
void alto2_cpu_device::f2_swrnrdy_1()
{
	diablo_hd_device* dhd = m_drive[m_dsk.drive];
	UINT16 r = dhd ? dhd->get_seek_read_write_0() : 1;
	UINT16 init = INIT ? 037 : 0;

	LOG((LOG_DISK,1,"	SWRNRDY; %sbranch (%#o|%#o|%#o)\n", (r | init) ? "" : "no ", m_next2, r, init));
	m_next2 |= r | init;
	m_dsk.wdinit0 = 0;
}

/**
 * @brief f2_nfer late: branch on the disk fatal error condition
 *
 * NEXT ← NEXT OR (if fatal error in latches ? 0 : 1)
 */
void alto2_cpu_device::f2_nfer_1()
{
	UINT16 r = m_dsk.kfer ? 0 : 1;
	UINT16 init = INIT ? 037 : 0;

	LOG((LOG_DISK,1,"	NFER; %sbranch (%#o|%#o|%#o)\n", (r | init) ? "" : "no ", m_next2, r, init));
	m_next2 |= r | init;
	m_dsk.wdinit0 = 0;
}

/**
 * @brief f2_strobon late: branch on the seek busy status
 *
 * NEXT ← NEXT OR (if seek strobe still on ? 1 : 0)
 * <PRE>
 * The STROBE signal is elongated with the help of two monoflops.
 * The first one has a rather short pulse duration:
 * 	tW = K * Rt * Cext * (1 + 0.7/Rt)
 *	K = 0.28 for 74123
 *	Rt = kOhms
 * 	Cext = pF
 * Rt = 20k, Cext = 150pf => 870ns
 *
 * The first one triggers the second, which will be cleared
 * by ADDRACK' from the drive going 0.
 * Its duration is:
 * Rt = 20k, Cext = 0.01µF (=10000pF) => 57960ns (~= 58µs)
 * </PRE>
 */
void alto2_cpu_device::f2_strobon_1()
{
	UINT16 r = m_dsk.strobe;
	UINT16 init = INIT ? 037 : 0;

	LOG((LOG_DISK,2,"	STROBON; %sbranch (%#o|%#o|%#o)\n", (r | init) ? "" : "no ", m_next2, r, init));
	m_next2 |= r | init;
	m_dsk.wdinit0 = 0;
}

/**
 * @brief update the disk controller with a new bitclk
 *
 * @param id timer id
 * @param arg bit number
 */
void alto2_cpu_device::disk_bitclk(void* ptr, INT32 arg)
{
	(void)ptr;
	diablo_hd_device* dhd = m_drive[m_dsk.drive];
	int bits = dhd->bits_per_sector();
	int clk = arg & 1;
	int bit = 0;

	/**
	 * The source for BITCLK and DATAIN depends on disk controller part #65
	 * <PRE>
	 *  BCLKSRC  W/R | BITCLK | DATAIN
	 * --------------+--------+---------
	 *    0       0  |  RDCLK | RDDATA
	 *    0       1  |  CLK/2 | DATOUT
	 *    1       0  |  CLK/2 | RDDATA
	 *    1       1  |  CLK/2 | DATOUT
	 * </PRE>
	 */
	if (m_dsk.krwc & RWC_WRITE) {
		bit = (m_dsk.shiftout >> 15) & 1;
		kwd_timing(clk, bit, 0);
		if (GET_KCOM_XFEROFF(m_dsk.kcom)) {
			/* do anything, if the transfer is off? */
		} else {
			LOG((LOG_DISK,7,"	BITCLK#%d bit:%d (write) @%lldns\n", arg, bit, ntime()));
			if (clk)
				dhd->wr_data(arg, bit);
			else
				dhd->wr_data(arg, 1);
		}
	} else if (GET_KCOM_BCLKSRC(m_dsk.kcom)) {
		/* always select the crystal clock */
		bit = dhd->rd_data(arg);
		LOG((LOG_DISK,7,"	BITCLK#%d bit:%d (read, crystal) @%lldns\n", arg, bit, ntime()));
		kwd_timing(clk, bit, 0);
	} else {
		/* if XFEROFF is set, keep the bit at 1 (RDGATE' is high) */
		if (GET_KCOM_XFEROFF(m_dsk.kcom)) {
			clk = dhd->rd_clock(arg);
			bit = 1;
		} else {
			bit = dhd->rd_data(arg);
			LOG((LOG_DISK,7,"	BITCLK#%d bit:%d (read, driveclk) @%lldns\n", arg, bit, ntime()));
		}
		kwd_timing(clk, bit, 0);
	}

	/* more bits to clock? */
	if (++arg < bits) {
#if	USE_BITCLK_TIMER
		m_dsk.bitclk_timer->adjust(dhd->bit_time(), arg);
#else
		if (!m_dsk.bitclk_time)
			m_dsk.bitclk_time = static_cast<int>(dhd->bit_time().as_double() * ATTOSECONDS_PER_NANOSECOND);
		m_bitclk_time += m_dsk.bitclk_time;
#endif
	}
}

/**
 * @brief callback is called by the drive timer whenever a new sector starts
 *
 * @param unit the unit number
 */
void alto2_cpu_device::next_sector(int unit)
{
	diablo_hd_device* dhd = m_drive[unit];
	LOG((LOG_DISK,0,"%s dhd=%p\n", __FUNCTION__, dhd));
#if	USE_BITCLK_TIMER
	if (m_dsk.bitclk_timer) {
		LOG((LOG_DISK,0,"	unit #%d stop bitclk\n", unit));
		m_dsk.bitclk_timer->reset();
	}
#else
	if (m_bitclk_time >= 0) {
		LOG((LOG_DISK,0,"	unit #%d stop bitclk\n", unit));
		m_bitclk_time = -1;
	}
#endif

	/* KSTAT[0-3] update the current sector in the kstat field */
	PUT_KSTAT_SECTOR(m_dsk.kstat, dhd->get_sector() ^ 017);

	/* clear input and output shift registers (?) */
	m_dsk.shiftin = 0;
	m_dsk.shiftout = 0;

	LOG((LOG_DISK,1,"	unit #%d sector %d start\n", unit, GET_KSTAT_SECTOR(m_dsk.kstat)));


#if	USE_BITCLK_TIMER
	// HACK: no command, no bit clock
	if (debug_read_mem(0521))
		/* start a timer chain for the bit clock */
		disk_bitclk(0, 0);
#else
	// TODO: verify current sector == requested sector and only then run the bitclk?
	// HACK: no command, no bit clock
	if (debug_read_mem(0521)) {
		// Make the CPU execution loop call disk_bitclk
		m_bitclk_time = 0;
		m_bitclk_index = 0;
	}
#endif
}

/**
 * @brief initialize the disk context and insert a disk wort timer
 *
 * @result returns 0 on success, fatal() on error
 */
void alto2_cpu_device::init_disk()
{
	memset(&m_dsk, 0, sizeof(m_dsk));

	/** @brief simulate previous sysclka */
	m_sysclka0[0] = JKFF_CLK;
	m_sysclka0[1] = JKFF_0;
	m_sysclka0[2] = JKFF_0;
	m_sysclka0[3] = JKFF_CLK;

	/** @brief simulate current sysclka */
	m_sysclka1[0] = JKFF_0;
	m_sysclka1[1] = JKFF_0;
	m_sysclka1[2] = JKFF_CLK;
	m_sysclka1[3] = JKFF_CLK;

	/** @brief simulate previous sysclkb */
	m_sysclkb0[0] = JKFF_CLK;
	m_sysclkb0[1] = JKFF_CLK;
	m_sysclkb0[2] = JKFF_0;
	m_sysclkb0[3] = JKFF_0;

	/** @brief simulate current sysclkb */
	m_sysclkb1[0] = JKFF_CLK;
	m_sysclkb1[1] = JKFF_0;
	m_sysclkb1[2] = JKFF_0;
	m_sysclkb1[3] = JKFF_CLK;

	m_dsk.wdtskena = 1;
	m_dsk.egate = 1;
	m_dsk.wrgate = 1;
	m_dsk.rdgate = 1;

	m_dsk.seclate = 0;
	m_dsk.ok_to_run = 0;

#if	USE_BITCLK_TIMER
	m_dsk.bitclk_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(alto2_cpu_device::disk_bitclk),this));
#endif

	m_dsk.strobon_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(alto2_cpu_device::disk_strobon),this));
	m_dsk.strobon_timer->adjust(attotime::from_nsec(TW_STROBON));

	m_dsk.seclate_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(alto2_cpu_device::disk_seclate),this));
	m_dsk.seclate_timer->adjust(attotime::from_nsec(TW_SECLATE), 1);

	m_dsk.ok_to_run_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(alto2_cpu_device::disk_ok_to_run),this));
	m_dsk.ok_to_run_timer->adjust(attotime::from_nsec(15 * ALTO2_UCYCLE), 1);

	m_dsk.ready_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(alto2_cpu_device::disk_ready_mf31a),this));
	m_dsk.ready_timer->reset();
}

