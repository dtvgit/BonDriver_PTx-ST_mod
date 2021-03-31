// ========================================================================================
//	EARTH_PT3.h
//	�o�[�W���� 0.96 (2012.07.09)
//	���o�[�W���� 1.0 �ɂȂ�܂ł̓C���^�[�t�F�[�X���ύX�ɂȂ�\��������܂��B
//	���o�[�W���� 0.91�`0.96 �܂ł̃C���^�[�t�F�[�X�͉��ʌ݊��ɂȂ��Ă��܂��B
// ========================================================================================

#ifndef _EARTH_PT3_H
#define _EARTH_PT3_H

#include "Prefix.h"

namespace EARTH3 {
namespace PT {
	class Device;
	class Device_;

	// +------------+
	// | �o�X�N���X |
	// +------------+
	// �o�X��̃f�o�C�X��񋓂��܂��B�܂��f�o�C�X�C���X�^���X�𐶐����܂��B
	class Bus {
	public:
		// [�@�\] Bus �C���X�^���X�𐶐�
		// [����] ���p�Ώۃh���C�o���� "windrvr6_EARTHSOFT_PT3" �ł��B
		// [�Ԓl] STATUS_INVALID_PARAM_ERROR �� ���� bus �� NULL
		//        STATUS_WDAPI_LOAD_ERROR    �� LoadLibrary(TEXT("wdapi1100.dll")) �̕Ԓl�� NULL
		//        STATUS_WD_DriverName_ERROR �� WD_DriverName() �̕Ԓl�� NULL
		//        STATUS_WD_Open_ERROR       �� WD_Open() �ŃG���[������
		//        STATUS_WD_Version_ERROR    �� WD_Version() �ŃG���[�������B�܂��̓o�[�W������ 11.1.0 �łȂ�
		//        STATUS_WD_License_ERROR    �� WD_License() �ŃG���[������
		typedef status (*NewBusFunction)(Bus **bus);

		// [�@�\] �C���X�^���X�����
		// [����] delete �͎g���܂���B���̊֐����Ăяo���Ă��������B
		// [�Ԓl] STATUS_ALL_DEVICES_MUST_BE_DELETED_ERROR �� NewDevice() �Ő������ꂽ�f�o�C�X���S�� Delete() ����Ă��Ȃ�
		virtual status Delete() = 0;

		// [�@�\] SDK �o�[�W�������擾
		// [����] �o�[�W������ 2.0 �̏ꍇ�A�l�� 0x200 �ɂȂ�܂��B
		//        ��� 24 �r�b�g�������ł���΃o�C�i���݊��ɂȂ�悤�ɓw�߂܂��̂ŁA
		//        ((version >> 8) == 2) �ł��邩���`�F�b�N���Ă��������B
		// [�Ԓl] STATUS_INVALID_PARAM_ERROR �� ���� version �� NULL
		virtual status GetVersion(uint32 *version) const = 0;

		// �f�o�C�X���
		struct DeviceInfo {
			uint32	Bus;		// PCI �o�X�ԍ�
			uint32	Slot;		// PCI �f�o�C�X�ԍ�
			uint32	Function;	// PCI �t�@���N�V�����ԍ� (���퓮�쎞�͕K�� 0 �ɂȂ�܂�)
			uint32	PTn;		// �i�� (PT3:3)
		};

		// [�@�\] �F������Ă���f�o�C�X�̃��X�g���擾
		// [����] PCI �o�X���X�L�������ăf�o�C�X�����X�g�A�b�v���܂��B
		//        deviceInfoCount �͌Ăяo���O�Ƀf�o�C�X�̏�������w�肵�܂��B�ďo����͌��������f�o�C�X����Ԃ��܂��B
		// [�Ԓl] STATUS_INVALID_PARAM_ERROR   �� ���� deviceInfoPtr, deviceInfoCount �̂����ꂩ�� NULL
		//        STATUS_WD_PciScanCards_ERROR �� WD_PciScanCards �ŃG���[������
		virtual status Scan(DeviceInfo *deviceInfoPtr, uint32 *deviceInfoCount) = 0;

		// [�@�\] �f�o�C�X�C���X�^���X�𐶐�����
		// [����] �f�o�C�X���\�[�X�̔r���`�F�b�N�͂��̊֐��ł͍s���܂���BDevice::Open() �ōs���܂��B
		//        Device_ �͔���J�C���^�[�t�F�[�X�ł��Bdevice_ �� NULL �ɂ��Ă��������B
		// [�Ԓl] STATUS_INVALID_PARAM_ERROR �� ���� deviceInfoPtr, device �̂����ꂩ�� NULL
		//                                      �܂��͈��� _device �� NULL �łȂ�
		virtual status NewDevice(const DeviceInfo *deviceInfoPtr, Device **device, Device_ **device_ = NULL) = 0;

	protected:
		virtual ~Bus() {}
	};

	// +----------------+
	// | �f�o�C�X�N���X |
	// +----------------+
	// ���̃C���X�^���X 1 ���{�[�h 1 ���ɑΉ����Ă��܂��B
	class Device {
	public:
		// ----
		// ���
		// ----

		// [�@�\] �C���X�^���X�����
		// [����] delete �͎g���܂���B���̊֐����Ăяo���Ă��������B
		// [�Ԓl] STATUS_DEVICE_MUST_BE_CLOSED_ERROR �� �f�o�C�X���I�[�v����ԂȂ̂ŃC���X�^���X������ł��Ȃ�
		virtual status Delete() = 0;

		// ------------------
		// �I�[�v���E�N���[�Y
		// ------------------

		// [�@�\] �f�o�C�X�̃I�[�v��
		// [����] �ȉ��̎菇�ɉ����čs���܂��B
		//        1. ���Ƀf�o�C�X���I�[�v������Ă��Ȃ������m�F����B
		//        2. ���r�W����ID (�R���t�B�M�����[�V������� �A�h���X 0x08) �� 0x01 �ł��邩�𒲂ׂ�B
		//        3. �R���t�B�M�����[�V������Ԃ̃f�o�C�X�ŗL���W�X�^�̈���g���� PCI �o�X�ł̃r�b�g�������Ȃ������m�F����B
		//        4. ���� SDK �Ő��䂪�\�� FPGA ��H�̃o�[�W�����ł��邩���m�F����B
		// [�Ԓl] STATUS_DEVICE_IS_ALREADY_OPEN_ERROR   �� �f�o�C�X�͊��ɃI�[�v������Ă���
		//        STATUS_WD_PciGetCardInfo_ERROR        �� WD_PciGetCardInfo() �ŃG���[������
		//        STATUS_WD_PciGetCardInfo_Bus_ERROR    �� �o�X��񐔂� 1 �ȊO
		//        STATUS_WD_PciGetCardInfo_Memory_ERROR �� ��������񐔂� 1 �ȊO
		//        STATUS_WD_CardRegister_ERROR          �� WD_CardRegister() �ŃG���[������
		//        STATUS_WD_PciConfigDump_ERROR         �� WD_PciConfigDump() �ŃG���[������
		//        STATUS_CONFIG_REVISION_ERROR          �� ���r�W����ID �� 0x01 �łȂ�
		//        STATUS_PCI_BUS_ERROR                  �� PCI �o�X�ł̃r�b�g����������
		//        STATUS_PCI_BASE_ADDRESS_ERROR         �� �R���t�B�M�����[�V������Ԃ� BaseAddress0 �� 0
		//        STATUS_FPGA_VERSION_ERROR             �� �Ή����Ă��Ȃ� FPGA ��H�o�[�W����
		//        STATUS_WD_CardCleanupSetup_ERROR      �� WD_CardCleanupSetup() �ŃG���[������
		//        STATUS_DCM_LOCK_TIMEOUT_ERROR         �� DCM ����莞�Ԍo�ߌ�����b�N��ԂɂȂ�Ȃ�
		//        STATUS_DCM_SHIFT_TIMEOUT_ERROR        �� DCM �̃t�F�[�Y�V�t�g����莞�Ԍo�ߌ���������Ȃ�
		virtual status Open() = 0;

		// [�@�\] �f�o�C�X�̃N���[�Y
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		virtual status Close() = 0;

		// ------------
		// �Œ���擾
		// ------------
		struct ConstantInfo {
			uint8	PTn;							// ���W�X�^ 0x00 [31:24]
			uint8	Version_RegisterMap;			// ���W�X�^ 0x00 [23:16]
			uint8	Version_FPGA;					// ���W�X�^ 0x00 [15: 8]
			bool	CanTransportTS;					// ���W�X�^ 0x0c [ 5]
			uint32	BitLength_PageDescriptorSize;	// ���W�X�^ 0x0c [ 4: 0]
		};

		virtual status GetConstantInfo(ConstantInfo *) const = 0;

		// ------------
		// �d���E������
		// ------------

		enum LnbPower {
			LNB_POWER_OFF,	// �I�t
			LNB_POWER_15V,	// 15V �o��
			LNB_POWER_11V	// 11V �o��
		};

		// [�@�\] LNB �d������
		// [����] �`���[�i�[�̓d���Ƃ͓Ɨ��ɐ���\�ł��B�f�t�H���g�l�� LNB_POWER_OFF �ł��B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� lnbPower �� NULL (GetLnbPower �̂�)
		virtual status SetLnbPower(LnbPower  lnbPower)       = 0;
		virtual status GetLnbPower(LnbPower *lnbPower) const = 0;

		// [�@�\] �f�o�C�X���N���[�Y�i�ُ�I���ɂƂ��Ȃ��N���[�Y���܂ށj���� LNB �d������
		// [����] �f�t�H���g�l�� LNB_POWER_OFF �ł��B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR  �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR       �� ���� lnbPower �� NULL (GetLnbPowerWhenClose �̂�)
		//        STATUS_WD_CardCleanupSetup_ERROR �� WD_CardCleanupSetup() �ŃG���[������ (SetLnbPowerWhenClose �̂�)
		virtual status SetLnbPowerWhenClose(LnbPower  lnbPower)       = 0;
		virtual status GetLnbPowerWhenClose(LnbPower *lnbPower) const = 0;

		// [�@�\] �`���[�i�[������
		// [����] �`���[�i�[�����������܂��B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_I2C_ERROR                �� ����IC ���烊�[�h�������W�X�^�l���ُ�
		virtual status InitTuner() = 0;

		// ��M����
		enum ISDB {
			ISDB_S,
			ISDB_T,

			ISDB_COUNT
		};

		// [�@�\] �`���[�i�[�ȓd�͐���
		// [����] �`���[�i�[������͏ȓd�̓I���ɂȂ��Ă��܂��̂ŁA��M�O�ɏȓd�͂��I�t�ɂ���K�v������܂��B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫�������� isdb ���͈͊O
		//                                           ���� sleep �� NULL (GetTunerSleep �̂�)
		virtual status SetTunerSleep(ISDB isdb, uint32 index, bool  sleep)       = 0;
		virtual status GetTunerSleep(ISDB isdb, uint32 index, bool *sleep) const = 0;

		// ----------
		// �ǔ����g��
		// ----------

		// [�@�\] �ǔ����g���̐���
		// [����] offset �Ŏ��g���̒������\�ł��B�P�ʂ� ISDB-S �̏ꍇ�� 1kHz�AISDB-T �̏ꍇ�� 1/7MHz �ł��B
		//        �Ⴆ�΁AC24 ��W����� 2MHz �������g���ɐݒ肷��ɂ� SetFrequency(tuner, ISDB_T, 23, 7*2) �Ƃ��܂��B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫�������� isdb ���͈͊O
		//                                           ���� channel �� NULL (GetFrequency �̂�)
		//        STATUS_TUNER_IS_SLEEP_ERROR     �� �`���[�i�[���ȓd�͏�Ԃ̂��ߐݒ�s�� (SetFrequency �̂�)
		virtual status SetFrequency(ISDB isdb, uint32 index, uint32  channel, sint32  offset = 0)       = 0;
		virtual status GetFrequency(ISDB isdb, uint32 index, uint32 *channel, sint32 *offset = 0) const = 0;

		// (ISDB-S)
		// +----+------+---------+ +----+------+---------+ +----+------+---------+
		// | ch | TP # | f (MHz) | | ch | TP # | f (MHz) | | ch | TP # | f (MHz) |
		// +----+------+---------+ +----+------+---------+ +----+------+---------+
		// |  0 | BS 1 | 1049.48 | | 12 | ND 2 | 1613.00 | | 24 | ND 1 | 1593.00 |
		// |  1 | BS 3 | 1087.84 | | 13 | ND 4 | 1653.00 | | 25 | ND 3 | 1633.00 |
		// |  2 | BS 5 | 1126.20 | | 14 | ND 6 | 1693.00 | | 26 | ND 5 | 1673.00 |
		// |  3 | BS 7 | 1164.56 | | 15 | ND 8 | 1733.00 | | 27 | ND 7 | 1713.00 |
		// |  4 | BS 9 | 1202.92 | | 16 | ND10 | 1773.00 | | 28 | ND 9 | 1753.00 |
		// |  5 | BS11 | 1241.28 | | 17 | ND12 | 1813.00 | | 29 | ND11 | 1793.00 |
		// |  6 | BS13 | 1279.64 | | 18 | ND14 | 1853.00 | | 30 | ND13 | 1833.00 |
		// |  7 | BS15 | 1318.00 | | 19 | ND16 | 1893.00 | | 31 | ND15 | 1873.00 |
		// |  8 | BS17 | 1356.36 | | 20 | ND18 | 1933.00 | | 32 | ND17 | 1913.00 |
		// |  9 | BS19 | 1394.72 | | 21 | ND20 | 1973.00 | | 33 | ND19 | 1953.00 |
		// | 10 | BS21 | 1433.08 | | 22 | ND22 | 2013.00 | | 34 | ND21 | 1993.00 |
		// | 11 | BS23 | 1471.44 | | 23 | ND24 | 2053.00 | | 35 | ND23 | 2033.00 |
		// +----+------+---------+ +----+------+---------+ +----+------+---------+
		//
		// (ISDB-T)
		// +-----+-----+---------+ +-----+-----+---------+ +-----+-----+---------+ +-----+-----+---------+ +-----+-----+---------+
		// | ch. | Ch. | f (MHz) | | ch. | Ch. | f (MHz) | | ch. | Ch. | f (MHz) | | ch. | Ch. | f (MHz) | | ch. | Ch. | f (MHz) |
		// +-----+-----+---------+ +-----+-----+---------+ +-----+-----+---------+ +-----+-----+---------+ +-----+-----+---------+
		// |   0 |   1 |  93+1/7 | |  23 | C24 | 231+1/7 | |  46 | C47 | 369+1/7 | |  69 |  19 | 509+1/7 | |  92 |  42 | 647+1/7 |
		// |   1 |   2 |  99+1/7 | |  24 | C25 | 237+1/7 | |  47 | C48 | 375+1/7 | |  70 |  20 | 515+1/7 | |  93 |  43 | 653+1/7 |
		// |   2 |   3 | 105+1/7 | |  25 | C26 | 243+1/7 | |  48 | C49 | 381+1/7 | |  71 |  21 | 521+1/7 | |  94 |  44 | 659+1/7 |
		// |   3 | C13 | 111+1/7 | |  26 | C27 | 249+1/7 | |  49 | C50 | 387+1/7 | |  72 |  22 | 527+1/7 | |  95 |  45 | 665+1/7 |
		// |   4 | C14 | 117+1/7 | |  27 | C28 | 255+1/7 | |  50 | C51 | 393+1/7 | |  73 |  23 | 533+1/7 | |  96 |  46 | 671+1/7 |
		// |   5 | C15 | 123+1/7 | |  28 | C29 | 261+1/7 | |  51 | C52 | 399+1/7 | |  74 |  24 | 539+1/7 | |  97 |  47 | 677+1/7 |
		// |   6 | C16 | 129+1/7 | |  29 | C30 | 267+1/7 | |  52 | C53 | 405+1/7 | |  75 |  25 | 545+1/7 | |  98 |  48 | 683+1/7 |
		// |   7 | C17 | 135+1/7 | |  30 | C31 | 273+1/7 | |  53 | C54 | 411+1/7 | |  76 |  26 | 551+1/7 | |  99 |  49 | 689+1/7 |
		// |   8 | C18 | 141+1/7 | |  31 | C32 | 279+1/7 | |  54 | C55 | 417+1/7 | |  77 |  27 | 557+1/7 | | 100 |  50 | 695+1/7 |
		// |   9 | C19 | 147+1/7 | |  32 | C33 | 285+1/7 | |  55 | C56 | 423+1/7 | |  78 |  28 | 563+1/7 | | 101 |  51 | 701+1/7 |
		// |  10 | C20 | 153+1/7 | |  33 | C34 | 291+1/7 | |  56 | C57 | 429+1/7 | |  79 |  29 | 569+1/7 | | 102 |  52 | 707+1/7 |
		// |  11 | C21 | 159+1/7 | |  34 | C35 | 297+1/7 | |  57 | C58 | 435+1/7 | |  80 |  30 | 575+1/7 | | 103 |  53 | 713+1/7 |
		// |  12 | C22 | 167+1/7 | |  35 | C36 | 303+1/7 | |  58 | C59 | 441+1/7 | |  81 |  31 | 581+1/7 | | 104 |  54 | 719+1/7 |
		// |  13 |   4 | 173+1/7 | |  36 | C37 | 309+1/7 | |  59 | C60 | 447+1/7 | |  82 |  32 | 587+1/7 | | 105 |  55 | 725+1/7 |
		// |  14 |   5 | 179+1/7 | |  37 | C38 | 315+1/7 | |  60 | C61 | 453+1/7 | |  83 |  33 | 593+1/7 | | 106 |  56 | 731+1/7 |
		// |  15 |   6 | 185+1/7 | |  38 | C39 | 321+1/7 | |  61 | C62 | 459+1/7 | |  84 |  34 | 599+1/7 | | 107 |  57 | 737+1/7 |
		// |  16 |   7 | 191+1/7 | |  39 | C40 | 327+1/7 | |  62 | C63 | 465+1/7 | |  85 |  35 | 605+1/7 | | 108 |  58 | 743+1/7 |
		// |  17 |   8 | 195+1/7 | |  40 | C41 | 333+1/7 | |  63 |  13 | 473+1/7 | |  86 |  36 | 611+1/7 | | 109 |  59 | 749+1/7 |
		// |  18 |   9 | 201+1/7 | |  41 | C42 | 339+1/7 | |  64 |  14 | 479+1/7 | |  87 |  37 | 617+1/7 | | 110 |  60 | 755+1/7 |
		// |  19 |  10 | 207+1/7 | |  42 | C43 | 345+1/7 | |  65 |  15 | 485+1/7 | |  88 |  38 | 623+1/7 | | 111 |  61 | 761+1/7 |
		// |  20 |  11 | 213+1/7 | |  43 | C44 | 351+1/7 | |  66 |  16 | 491+1/7 | |  89 |  39 | 629+1/7 | | 112 |  62 | 767+1/7 |
		// |  21 |  12 | 219+1/7 | |  44 | C45 | 357+1/7 | |  67 |  17 | 497+1/7 | |  90 |  40 | 635+1/7 | +-----+-----+---------+
		// |  22 | C23 | 225+1/7 | |  45 | C46 | 363+1/7 | |  68 |  18 | 503+1/7 | |  91 |  41 | 641+1/7 |
		// +-----+-----+---------+ +-----+-----+---------+ +-----+-----+---------+ +-----+-----+---------+
		//
		// C24�`C27 �́A�P�[�u���e���r�ǂɂ�艺�L�̎��g���ő��M����Ă���ꍇ������܂��B
		// +-----+---------+
		// | Ch. | f (MHz) |
		// +-----+---------+
		// | C24 | 233+1/7 |
		// | C25 | 239+1/7 |
		// | C26 | 245+1/7 |
		// | C27 | 251+1/7 |
		// +-----+---------+

		// ----------
		// ���g���덷
		// ----------

		// [�@�\] ���g���덷���擾
		// [����] �l�̈Ӗ��͎��̒ʂ�ł��B
		//        �N���b�N���g���덷: clock/100 (ppm)
		//        �L�����A���g���덷: carrier (Hz)
		//        �����g�̎��g�����x�͏\���ɍ������肷��ƁA�덷����������v�f�Ƃ��Ĉȉ��̂悤�Ȃ��̂��l�����܂��B
		//        (ISDB-S) LNB �ł̎��g���ϊ����x / �q���� PLL-IC �ɐڑ�����Ă���U���q�̐��x / ���� IC �ɐڑ�����Ă���U���q�̐��x
		//        (ISDB-T) �n�㑤 PLL-IC �ɐڑ�����Ă���U���q�̐��x / ���� IC �ɐڑ�����Ă���U���q�̐��x
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫�������� isdb ���͈͊O
		//                                           ���� clock, carrier �̂����ꂩ�� NULL
		//        STATUS_TUNER_IS_SLEEP_ERROR     �� �`���[�i�[���ȓd�͏��
		virtual status GetFrequencyOffset(ISDB isdb, uint32 tuner, sint32 *clock, sint32 *carrier) = 0;

		// --------
		// C/N�EAGC
		// --------

		// [�@�\] C/N �� AGC ���擾
		// [����] C/N �͒჌�C�e���V�ő���ł��邽�߁A�A���e�i�̌����𒲐�����̂ɕ֗��ł��B
		//        �l�̈Ӗ��͎��̒ʂ�ł��B
		//        C/N                : cn100/100 (dB)
		//        ���݂� AGC �l      : currentAgc
		//        �����ő厞�� AGC �l: maxAgc
		//        currentAgc �͈̔͂� 0 ���� maxAgc �܂łł��B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫�������� isdb ���͈͊O
		//                                           ���� cn100, currentAgc, maxAgc �̂����ꂩ�� NULL
		//        STATUS_TUNER_IS_SLEEP_ERROR     �� �`���[�i�[���ȓd�͏��
		virtual status GetCnAgc(ISDB isdb, uint32 tuner, uint32 *cn100, uint32 *currentAgc, uint32 *maxAgc) = 0;

		// ----------------------
		// RF Level (ISDB-T �̂�)
		// ----------------------
		virtual status GetRFLevel(uint32 tuner, float *level) = 0;

		// -------------------
		// TS-ID (ISDB-S �̂�)
		// -------------------

		// [�@�\] TS-ID ��ݒ�
		// [����] �ݒ�l������IC �̓���ɔ��f�����܂Ŏ��Ԃ��|����܂��B
		//        GetLayerS() ���Ăяo���O�ɁAGetIdS() ���g���Đ؂�ւ��������������Ƃ��m�F���Ă��������B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫��
		virtual status SetIdS(uint32 tuner, uint32 id) = 0;

		// [�@�\] ���ݏ������� TS-ID ���擾
		// [����] GetLayerS() �Ŏ擾�ł��郌�C�����́A���̊֐��Ŏ������ TS-ID �̂��̂ɂȂ�܂��B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫���B�܂��͈��� id �� NULL
		virtual status GetIdS(uint32 tuner, uint32 *id) = 0;

		// ------------
		// �G���[���[�g
		// ------------

		// �K�w�C���f�b�N�X
		enum LayerIndex {
			// ISDB-S
			LAYER_INDEX_L = 0,	// ��K�w
			LAYER_INDEX_H,		// ���K�w

			// ISDB-T
			LAYER_INDEX_A = 0,	// A �K�w
			LAYER_INDEX_B,		// B �K�w
			LAYER_INDEX_C		// C �K�w
		};

		// �K�w��
		enum LayerCount {
			// ISDB-S
			LAYER_COUNT_S = LAYER_INDEX_H + 1,

			// ISDB-T
			LAYER_COUNT_T = LAYER_INDEX_C + 1
		};

		// �G���[���[�g
		struct ErrorRate {
			uint32 Numerator, Denominator;
		};

		// [�@�\] GetInnerErrorRate() �őΏۂƂȂ�K�w��ݒ�
		virtual status SetInnerErrorRateLayer(ISDB isdb, uint32 tuner, LayerIndex layerIndex) = 0;

		// [�@�\] �������Œ������ꂽ�G���[���[�g���擾
		virtual status GetInnerErrorRate(ISDB isdb, uint32 tuner, ErrorRate *errorRate) = 0;

		// [�@�\] ���[�h�\�����������Œ������ꂽ�G���[���[�g���擾
		// [����] ����Ɏ��Ԃ��|����܂����A��M�i���𐳊m�ɔc������ɂ� C/N �ł͂Ȃ����̃G���[���[�g���Q�l�ɂ��Ă��������B
		//        �ЂƂ̖ڈ��Ƃ��� 2�~10^-4 �ȉ��ł���΁A���[�h�\������������ɂقڃG���[�t���[�ɂȂ�Ƃ����Ă��܂��B
		//        �G���[���[�g�̏W�v�P�ʂ͎��̒ʂ�ł��B
		//        ISDB-S: 1024 �t���[��
		//        ISDB-T: 32 �t���[�� (���[�h 1,2) / 8 �t���[�� (���[�h 3)
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner, isdb, layerIndex ���͈͊O�B�܂��� errorRate �� NULL
		virtual status GetCorrectedErrorRate(ISDB isdb, uint32 tuner, LayerIndex layerIndex, ErrorRate *errorRate) = 0;

		// [�@�\] ���[�h�\�����������Œ������ꂽ�G���[���[�g���v�Z���邽�߂̃G���[�J�E���^��������
		// [����] �S�K�w�̃J�E���^�����������܂��B����̊K�w�̃J�E���^�����Z�b�g���邱�Ƃ͂ł��܂���B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫�������� isdb ���͈͊O
		virtual status ResetCorrectedErrorCount(ISDB isdb, uint32 tuner) = 0;

		// [�@�\] ���[�h�\�����������Œ���������Ȃ����� TS �p�P�b�g�����擾
		// [����] 0xffffffff �̎��� 0x00000000 �ɂȂ�܂��B
		//        TS �p�P�b�g�� 2nd Byte MSB �𐔂��Ă��������l�ɂȂ�܂��B
		//        ���̃J�E���^�� DMA �]���J�n���ɏ���������܂��B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫�������� isdb ���͈͊O�B�܂��͈��� count �� NULL
		virtual status GetErrorCount(ISDB isdb, uint32 tuner, uint32 *count) = 0;

		// --------------------------
		// TMCC�E���C���[�E���b�N����
		// --------------------------

		// ISDB-S TMCC ���
		// (�Q�l) STD-B20 2.9 TMCC���̍\�� �` 2.11 TMCC���̍X�V
		struct TmccS {
			uint32 Indicator;	// �ύX�w�� (5�r�b�g)
			uint32 Mode[4];		// �`�����[�hn (4�r�b�g)
			uint32 Slot[4];		// �`�����[�hn�ւ̊����X���b�g�� (6�r�b�g)
								// [����TS�^�X���b�g���͎擾�ł��܂���]
			uint32 Id[8];		// ����TS�ԍ�n�ɑ΂���TS ID (16�r�b�g)
			uint32 Emergency;	// �N������M�� (1�r�b�g)
			uint32 UpLink;		// �A�b�v�����N������ (4�r�b�g)
			uint32 ExtFlag;		// �g���t���O (1�r�b�g)
			uint32 ExtData[2];	// �g���̈� (61�r�b�g)
		};

		// [�@�\] ISDB-S �� TMCC �����擾
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫�������� tmcc �� NULL
		virtual status GetTmccS(uint32 tuner, TmccS *tmcc) = 0;

		// ISDB-S �K�w���
		struct LayerS {
			uint32 Mode [LAYER_COUNT_S];	// �`�����[�h (3�r�b�g)
			uint32 Count[LAYER_COUNT_S];	// �_�~�[�X���b�g���܂߂������X���b�g�� (6�r�b�g)
		};

		// [�@�\] ISDB-S �̃��C�������擾
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫�������� layerS �� NULL
		virtual status GetLayerS(uint32 tuner, LayerS *layerS) = 0;

		// ISDB-T TMCC ���
		// (�Q�l) STD-B31 3.15.6 TMCC��� �` 3.15.6.8 �Z�O�����g��
		struct TmccT {
			uint32 System;						// �V�X�e������ (2�r�b�g)
			uint32 Indicator;					// �`���p�����[�^�؂�ւ��w�W (4�r�b�g)
			uint32 Emergency;					// �ً}�x������p�N���t���O (1�r�b�g)
												// �J�����g���
			uint32 Partial;						// ������M�t���O (1�r�b�g)
												// �K�w���
			uint32 Mode      [LAYER_COUNT_T];	// �L�����A�ϒ����� (3�r�b�g)
			uint32 Rate      [LAYER_COUNT_T];	// �􍞂ݕ������� (3�r�b�g)
			uint32 Interleave[LAYER_COUNT_T];	// �C���^�[���[�u�� (3�r�b�g)
			uint32 Segment   [LAYER_COUNT_T];	// �Z�O�����g�� (4�r�b�g)
												// [�l�N�X�g���͎擾�ł��܂���]
			uint32 Phase;						// �A�����M�ʑ��␳�� (3�r�b�g)
			uint32 Reserved;					// ���U�[�u (12�r�b�g)
		};

		// [�@�\] ISDB-T �� TMCC �����擾
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫�������� tmcc �� NULL
		virtual status GetTmccT(uint32 tuner, TmccT *tmcc) = 0;

		// [�@�\] ISDB-T ���b�N������擾
		// [����] ���C�������݂��A�Ȃ������̃��C�����G���[�t���[�ł���Ƃ��� true �ɂȂ�܂��B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫�������� locked �� NULL
//		virtual status GetLockedT(uint32 tuner, bool locked[LAYER_COUNT_T]) = 0;

		// [�@�\] ISDB-T ���̓��̓A���v�̓d������
		virtual status SetAmpPowerT(bool b) = 0;

		// ��M�K�w
		enum LayerMask {
			LAYER_MASK_NONE,

			// ISDB-S
			LAYER_MASK_L = 1 << LAYER_INDEX_L,
			LAYER_MASK_H = 1 << LAYER_INDEX_H,

			// ISDB-T
			LAYER_MASK_A = 1 << LAYER_INDEX_A,
			LAYER_MASK_B = 1 << LAYER_INDEX_B,
			LAYER_MASK_C = 1 << LAYER_INDEX_C
		};

		// [�@�\] ��M�K�w�̐ݒ�
		// [����] ISDB-S �̒�K�w����M���Ȃ��悤�ɐݒ肷�邱�Ƃ͂ł��܂���B
		// [�Ԓl] STATUS_DEVICE_IS_NOT_OPEN_ERROR �� �f�o�C�X���I�[�v������Ă��Ȃ�
		//        STATUS_INVALID_PARAM_ERROR      �� ���� tuner �� 1 ���傫�������� isdb ���͈͊O
		//                                           ���� layerMask ���͈͊O (SetLayerEnable �̂�)
		//                                           ���� layerMask �� NULL (GetLayerEnable �̂�)
		virtual status SetLayerEnable(ISDB isdb, uint32 tuner, LayerMask  layerMask)       = 0;
		virtual status GetLayerEnable(ISDB isdb, uint32 tuner, LayerMask *layerMask) const = 0;

		// -------------
		// TS �s���e�X�g
		// -------------
		enum TsPinMode {
			TS_PIN_MODE_NORMAL,
			TS_PIN_MODE_LOW,
			TS_PIN_MODE_HIGH
		};

		struct TsPinsMode {
			TsPinMode	clock_data,
						byte,
						valid;
		};

		struct TsPinsLevel {
			bool	clock,	// �g�O������
					data,
					byte,
					valid;
		};

		virtual status SetTsPinsMode (ISDB isdb, uint32 tuner, const TsPinsMode  *mode ) = 0;
		virtual status GetTsPinsLevel(ISDB isdb, uint32 tuner,       TsPinsLevel *level) = 0;

		virtual status GetTsSyncByte(ISDB isdb, uint32 tuner, uint8 *syncByte) = 0;

		enum RamPinsMode {
			RAM_PINS_MODE_NORMAL,
			RAM_PINS_MODE_LOW,
			RAM_PINS_MODE_HIGH
		};

		virtual status SetRamPinsMode(RamPinsMode mode) = 0;

		// ------------------
		// DMA �]���p�o�b�t�@
		// ------------------

		// ���̊֐��͔p�~����܂����BLockBuffer() �������p���������B
		virtual status LockBuffer__Obsolated__(void *ptr, uint32 size, void **handle) = 0;
		virtual status UnlockBuffer(void *handle) = 0;

		struct BufferInfo {
			uint64 Address;		// �����A�h���X
			uint32 Size;		// �T�C�Y
		};

		virtual status GetBufferInfo(void *handle, const BufferInfo **infoTable, uint32 *infoCount) = 0;

		// --------
		// DMA �]��
		// --------

		// [�@�\] DMA �J�n�E��~�̐���
		// [����] DMA �]���͑S�� CPU ����݂��邱�ƂȂ����삵�܂��B
		//        GetTransferEnabled() �� true  ��������Ƃ��� SetTransferEnabled(true ) �Ƃ�����A
		//        GetTransferEnabled() �� false ��������Ƃ��� SetTransferEnabled(false) �Ƃ���ƃG���[�ɂȂ�܂��B
		//
		//        GetTransferEnabled() �Ŏ擾�ł���l�́A�P�� SetTransferEnabled() �ōŌ�ɐݒ肳�ꂽ�l�Ɠ����ł��B
		//        �]���J�E���^�� 0 �ɂȂ�ȂǁA�n�[�h�E�F�A���� DMA �]���������I�ɒ�~����v��������������܂����A
		//        ���̏ꍇ�ł� GetTransferEnabled() �œ�����l�͕ς��܂���B
		virtual status SetTransferPageDescriptorAddress(ISDB isdb, uint32 tunerIndex, uint64 pageDescriptorAddress) = 0;
		virtual status SetTransferEnabled              (ISDB isdb, uint32 tunerIndex, bool  enabled)       = 0;
		virtual status GetTransferEnabled              (ISDB isdb, uint32 tunerIndex, bool *enabled) const = 0;

		// resetError �� TS �G���[�p�P�b�g�̃��Z�b�g�ł��B�����I�ɂ͕ʊ֐��Ɉړ����܂��B
		virtual status SetTransferTestMode(ISDB isdb, uint32 tunerIndex, bool testMode = false, uint16 initial = 0, bool not = false/*, bool resetError = false*/) = 0;

		struct TransferInfo {
			bool	Busy;
			uint32	Status;						// 4�r�b�g
			bool	InternalFIFO_A_Overflow,
					InternalFIFO_A_Underflow;
			bool	ExternalFIFO_Overflow;
			uint32	ExternalFIFO_MaxUsedBytes;	// ExternalFIFO_Overflow �� false �̏ꍇ�̂ݗL��
			bool	InternalFIFO_B_Overflow,
					InternalFIFO_B_Underflow;
		};

		virtual status GetTransferInfo(ISDB isdb, uint32 tunerIndex, TransferInfo *transferInfo) = 0;

		// ---------------------
		// 0.96 �Œǉ����ꂽ�֐�
		// ---------------------

		enum TransferDirection {				// DMA �̓]������
			TRANSFER_DIRECTION_WRITE = 1 << 0,	// PCI �f�o�C�X���������Ƀf�[�^����������
			TRANSFER_DIRECTION_READ	 = 1 << 1,	// PCI �f�o�C�X���������̃f�[�^��ǂݍ���
			TRANSFER_DIRECTION_WRITE_READ = TRANSFER_DIRECTION_WRITE | TRANSFER_DIRECTION_READ
		};

		// [�@�\] �������̈�𕨗��������ɌŒ肷��
		// [����] DMA �]���p�������̈�� DMA �]�����J�n����O�ɕ����������ɌŒ肷��K�v������܂��B
		//        ptr �� size �Ń������̈���w�肵�Adirection �� DMA �]���������w�肵�܂��B
		//        handle �ŕԂ��ꂽ�|�C���^�� UnlockBuffer(), SyncBufferCpu(), SyncBufferDevice() �Ŏg�p���܂��B
		virtual status LockBuffer(void *ptr, uint32 size, TransferDirection direction, void **handle) = 0;

		// [�@�\] CPU �L���b�V���� DMA �]���p�������̈�̓��������
		// [����] ���L�ɋL�ڂ���s�������N����Ȃ��悤�ɁA���̊֐����Ă�œ��������K�v��������܂��B
		//        (�P�[�X1)
		//        1. CPU �� DMA �]���p�������̈�Ƀf�[�^����������
		//        2. �������܂ꂽ�f�[�^�� CPU �L���b�V����ɑ��݂��邾���� DMA �]���p�������̈�ɂ͖����������܂�Ă��Ȃ�
		//        3. PCI �f�o�C�X�� DMA �]���p�������̈�Ƀf�[�^����������
		//        4. CPU ���L���b�V����ɑ��݂���f�[�^�� DMA �]���p�������̈�ɏ������� (�s��������)
		//        ���s�������������Ȃ��悤�� 2 �̌�ɂ��̊֐����Ă�ł��������B
		//
		//        (�P�[�X2)
		//        1. PCI �f�o�C�X�� DMA �]���p�������̈�Ƀf�[�^����������
		//        2. CPU ���]���p����������f�[�^��ǂݍ���
		//        3. �ǂݍ��񂾃f�[�^�� CPU �̃L���b�V���ɕۑ������
		//        4. PCI �f�o�C�X�� DMA �]���p�������̈�Ɂu�V�����v�f�[�^����������
		//        5. CPU ���]���p����������f�[�^��ǂݍ���
		//        6. CPU �� DMA �]���p�������̈�ɂ���u�V�����v�f�[�^�ł͂Ȃ��A�L���b�V�����ꂽ�Â��f�[�^��ǂݍ��� (�s��������)
		//        ���s�������������Ȃ��悤�� 5 �̑O�ɂ��̊֐����Ă�ł��������B
		virtual status SyncBufferCpu(void *handle) = 0;

		// [�@�\] I/O �L���b�V���� DMA �]���p�������̈�̓��������
		// [����] ���L�ɋL�ڂ���s�������N����Ȃ��悤�ɁA���̊֐����Ă�œ��������K�v��������܂��B
		//        1. PCI �f�o�C�X�� DMA �]���p�������̈�Ƀf�[�^���������ނ��߂Ƀp�P�b�g�𑗏o����
		//        2. �p�P�b�g���󂯎�����f�C�o�X�͌�Ńf�[�^�����̃f�o�C�X�ɑ��o���邽�߁A���g�Ńf�[�^���L���b�V������
		//        3. ����ăf�[�^�͖��� DMA �]���p�������̈�ɏ������܂�Ă��Ȃ�
		//        4. CPU �� PCI �f�o�C�X���u�]���I���i���p�P�b�g���o�I���j�v�ł��邱�Ƃ��m�F����
		//        5. DMA �]���p�������̈悩��f�[�^��ǂݍ��� (�s��������)
		//        ���s�������������Ȃ��悤�� 5 �̑O�ɂ��̊֐����Ă�ł��������B
		virtual status SyncBufferIo(void *handle) = 0;

	protected:
		virtual ~Device() {}
	};

	enum Status {
		// �G���[�Ȃ�
		STATUS_OK,

		// ��ʓI�ȃG���[
		STATUS_GENERAL_ERROR = (1)*0x100,
		STATUS_NOT_IMPLIMENTED,
		STATUS_INVALID_PARAM_ERROR,
		STATUS_OUT_OF_MEMORY_ERROR,
		STATUS_INTERNAL_ERROR,

		// �o�X�N���X�̃G���[
		STATUS_WDAPI_LOAD_ERROR = (2)*256,	// wdapi1100.dll �����[�h�ł��Ȃ�
		STATUS_ALL_DEVICES_MUST_BE_DELETED_ERROR,

		// �f�o�C�X�N���X�̃G���[
		STATUS_PCI_BUS_ERROR = (3)*0x100,
		STATUS_CONFIG_REVISION_ERROR,
		STATUS_FPGA_VERSION_ERROR,
		STATUS_PCI_BASE_ADDRESS_ERROR,
		STATUS_FLASH_MEMORY_ERROR,

		STATUS_DCM_LOCK_TIMEOUT_ERROR,
		STATUS_DCM_SHIFT_TIMEOUT_ERROR,

		STATUS_POWER_RESET_ERROR,
		STATUS_I2C_ERROR,
		STATUS_TUNER_IS_SLEEP_ERROR,

		STATUS_PLL_OUT_OF_RANGE_ERROR,
		STATUS_PLL_LOCK_TIMEOUT_ERROR,

		STATUS_VIRTUAL_ALLOC_ERROR,
		STATUS_DMA_ADDRESS_ERROR,
		STATUS_BUFFER_ALREADY_ALLOCATED_ERROR,

		STATUS_DEVICE_IS_ALREADY_OPEN_ERROR,
		STATUS_DEVICE_IS_NOT_OPEN_ERROR,

		STATUS_BUFFER_IS_IN_USE_ERROR,
		STATUS_BUFFER_IS_NOT_ALLOCATED_ERROR,

		STATUS_DEVICE_MUST_BE_CLOSED_ERROR,

		// WinDriver �֘A�̃G���[
		STATUS_WD_DriverName_ERROR = (4)*0x100,

		STATUS_WD_Open_ERROR,
		STATUS_WD_Close_ERROR,

		STATUS_WD_Version_ERROR,
		STATUS_WD_License_ERROR,

		STATUS_WD_PciScanCards_ERROR,

		STATUS_WD_PciConfigDump_ERROR,

		STATUS_WD_PciGetCardInfo_ERROR,
		STATUS_WD_PciGetCardInfo_Bus_ERROR,
		STATUS_WD_PciGetCardInfo_Memory_ERROR,

		STATUS_WD_CardRegister_ERROR,
		STATUS_WD_CardUnregister_ERROR,

		STATUS_WD_CardCleanupSetup_ERROR,

		STATUS_WD_DMALock_ERROR,
		STATUS_WD_DMAUnlock_ERROR,

		STATUS_WD_DMASyncCpu_ERROR,
		STATUS_WD_DMASyncIo_ERROR,

		// ROM
		STATUS_ROM_ERROR = (5)*0x100,
		STATUS_ROM_TIMEOUT
	};
}
}

#endif
