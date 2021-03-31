#ifndef _EX_Buffer_H
#define _EX_Buffer_H

#include "Prefix.h"
#include "EARTH_PT3.h"
#include "EX_Utility.h"
#include "OS_Memory.h"
#include <vector>

namespace EARTH3 {
namespace EX {
	class Buffer {
	public:
		Buffer(PT::Device *device) : mDevice(device)
		{
			for (uint32 i=0; i<sizeof(mInfo)/sizeof(*mInfo); i++) {
				Info	*info = &mInfo[i];
				info->memory      = NULL;
				info->ptr         = NULL;
				info->size        = 0;
			}
		}

		~Buffer()
		{
			Free();
		}

		status Alloc(uint32 blockSize, uint32 blockCount, bool loop = true)
		{
			status	status;

			status = AllocMemory(blockSize, blockCount);
			if (status) return status;

			BuildPageDescriptor(loop);

			status = mDevice->SyncBufferCpu(mInfo[Index_Page].physicals[0].handle);
			if (status) return status;

			return status;
		}

		status Free()
		{
			status	status;
			status = FreeMemory();
			if (status) return status;

			return status;
		}

		status SyncCpu(uint32 blockIndex)
		{
			return mDevice->SyncBufferCpu(mInfo[Index_TS].physicals.at(blockIndex).handle);
		}

		status SyncIo(uint32 blockIndex)
		{
			return mDevice->SyncBufferIo(mInfo[Index_TS].physicals.at(blockIndex).handle);
		}

		void *Ptr(uint32 blockIndex)
		{
			Info	*info = &mInfo[Index_TS];
			uint8	*ptr = static_cast<uint8 *>(info->ptr);
			ptr += info->blockSize * blockIndex;
			return ptr;
		}

		uint64 PageDescriptorAddress()
		{
			_ASSERT(mInfo[Index_Page].physicals[0].bufferInfo);
			uint64	address = mInfo[Index_Page].physicals[0].bufferInfo[0].Address;

			return address;
		}

	protected:
		enum Index {
			Index_TS,		// TS �f�[�^�̈�
			Index_Page,		// �y�[�W�L�q�q�̈�
			Index_Count
		};

		struct Physical {
			void							*handle;
			const PT::Device::BufferInfo	*bufferInfo;
			uint32							bufferCount;
		};

		struct Info {
			OS::Memory				*memory;
			void					*ptr;
			uint32					size;
			uint32					blockSize;
			uint32					blockCount;
			std::vector<Physical>	physicals;
		};

		PT::Device		*const mDevice;
		Info			mInfo[Index_Count];

		status AllocMemory(uint32 blockSize, uint32 blockCount)
		{
			status	status = PT::STATUS_OK;

			// TS �f�[�^�̈���m�ۂ���
			status = AllocMemory(Index_TS, blockSize , blockCount);
			if (status) return status;

			// �y�[�W�L�q�q�̈���m�ۂ���
			// �L�q�q�̃T�C�Y�� 20 �o�C�g�Ȃ̂ŁA4096 �o�C�g�̗̈�ɍ\�z�ł���L�q�q�̐��� 204 �ł���B
			uint32	pageCount = (mInfo[Index_TS].size / OS::Memory::PAGE_SIZE + 203) / 204;
			status = AllocMemory(Index_Page, OS::Memory::PAGE_SIZE * pageCount, 1);
			if (status) return status;

			return status;
		}

		status AllocMemory(Index index, uint32 blockSize, uint32 blockCount)
		{
			status	status = PT::STATUS_OK;

			_ASSERT(index < Index_Count);
			Info	*info = &mInfo[index];

			// ���������m�ۂ���
			uint32	size = blockSize * blockCount;

			_ASSERT(info->memory == NULL);
			info->memory = new OS::Memory(size);
			if (info->memory == NULL) return PT::STATUS_OUT_OF_MEMORY_ERROR;

			uint8	*ptr = static_cast<uint8 *>(info->memory->Ptr());
			info->ptr        = ptr;
			info->size       = size;
			info->blockSize  = blockSize;
			info->blockCount = blockCount;
			if (ptr == NULL) return PT::STATUS_OUT_OF_MEMORY_ERROR;

			Physical	block;
			block.handle      = NULL;
			block.bufferInfo  = NULL;
			block.bufferCount = 0;

			// DMA �]�����ł���悤�Ƀu���b�N���ɕ����������ɌŒ肷��
			PT::Device::TransferDirection	direction = (index == Index_TS) ?
				PT::Device::TRANSFER_DIRECTION_WRITE : PT::Device::TRANSFER_DIRECTION_READ;

			uint32	offset = 0;
			for (uint32 i=0; i<blockCount; i++) {
				status = mDevice->LockBuffer(ptr + offset, blockSize, direction, &block.handle);
				if (U::Error(L"Device::LockBuffer()", status)) return status;

				status = mDevice->GetBufferInfo(block.handle, &block.bufferInfo, &block.bufferCount);
				if (U::Error(L"Device::GetBufferInfo()", status)) return status;

				info->physicals.push_back(block);

				offset += blockSize;
			}

			return status;
		}

		status FreeMemory()
		{
			status	status = PT::STATUS_OK;

			for (uint32 i=0; i<sizeof(mInfo)/sizeof(*mInfo); i++) {
				Info	*info = &mInfo[i];
				std::vector<Physical>	&physicals = info->physicals;
				for (uint32 i=0; i<physicals.size(); i++) {
					Physical	*block = &physicals[i];
					if (block->handle) {
						status = mDevice->UnlockBuffer(block->handle);
						if (U::Error(L"Device::UnlockBuffer()", status)) return status;
					}
				}
				physicals.clear();

				if (info->memory) {
					delete_(info->memory);
					info->memory = NULL;
					info->ptr    = NULL;
					info->size   = 0;
				}
			}

			return status;
		}

		// ���̃T���v���ł� 4096 �o�C�g�ɂ� 1�̋L�q�q���\�z���Ă��܂��B
		// OS::Memory �Ŋm�ۂ��郁�����T�C�Y�� 4096 �̔{���ŁAOS::Memory
		// �Ŏ擾�ł��鉼�z�A�h���X�� 4096 �o�C�g���E�ɂȂ��Ă���̂ŁA
		// FPGA.txt �ɋL�q����Ă���u4GB ���ׂ����y�[�W�̈�� 2�ɕ�������v
		// �����͍l������K�v���Ȃ��Ȃ�܂��B
		void BuildPageDescriptor(bool loop)
		{
			void							*pagePtr       = mInfo[Index_Page].ptr;
			std::vector<Physical>			&pagePhysicals = mInfo[Index_Page].physicals;
			const PT::Device::BufferInfo	*pageInfo      = pagePhysicals[0].bufferInfo;

			uint64	pageAddress = pageInfo->Address;
			uint32	pageSize    = pageInfo->Size;
			pageInfo++;

			// ���[�v�p�ɐ擪�̕����A�h���X��ۑ����Ă���
			uint64	loopPageAddress = pageAddress;

			void	*prevPagePtr = NULL;

			std::vector<Physical>	&tsPhysicals = mInfo[Index_TS].physicals;
			for (uint32 i=0; i<tsPhysicals.size(); i++) {
				Physical						*physical = &tsPhysicals[i];
				const PT::Device::BufferInfo	*tsInfo   = physical->bufferInfo;
				uint32							tsCount   = physical->bufferCount;

				for (uint32 j=0; j<tsCount; j++) {
					uint64	tsAddress = tsInfo->Address;
					uint32	tsSize    = tsInfo->Size;
					tsInfo++;

					_ASSERT((tsSize % OS::Memory::PAGE_SIZE) == 0);
					for (uint32 k=0; k<tsSize/OS::Memory::PAGE_SIZE; k++) {
						// �c�肪 20 �o�C�g�������ǂ������`�F�b�N
						if (pageSize < 20) {
							// �c�蕔�����X�L�b�v
							pagePtr = static_cast<uint8 *>(pagePtr) + pageSize;

							// �A�h���X�ƃT�C�Y���X�V
							pageAddress = pageInfo->Address;
							pageSize    = pageInfo->Size;
							pageInfo++;
						}

						// �O�̋L�q�q������΁A���̋L�q�q�ƂȂ���
						if (prevPagePtr) {
							LinkPageDescriptor(pageAddress, prevPagePtr);
						}

						// ���̋L�q�q�|�C���^��ۑ�����
						prevPagePtr = pagePtr;

						// �L�q�q���\�z���� (nextAddress �͌����_�ł͂킩��Ȃ����� 0 �ɂ��Ă���)
						WritePageDescriptor(tsAddress, OS::Memory::PAGE_SIZE, 0, &pagePtr);

						tsAddress += OS::Memory::PAGE_SIZE;

						pageAddress += 20;
						pageSize    -= 20;
					}
				}
			}

			if (prevPagePtr) {
				if (loop) {
					// ��������擪�փ��[�v������
					LinkPageDescriptor(loopPageAddress, prevPagePtr);
				} else {
					// ������ DMA �]�����I��������
					LinkPageDescriptor(1, prevPagePtr);
				}
			}
		}

		void WritePageDescriptor(uint64 address, uint32 size, uint64 nextAddress, void **ptr_)
		{
			_ASSERT(ptr_);
			uint8	*ptr = static_cast<uint8 *>(*ptr_);
			_ASSERT(ptr);

			*reinterpret_cast<uint64 *>(ptr +  0) = address     | 7;	// [2:0] �͖��������
			*reinterpret_cast<uint32 *>(ptr +  8) = size        | 7;	// [2:0] �͖��������
			*reinterpret_cast<uint64 *>(ptr + 12) = nextAddress | 2;	// [1  ] �͖��������

			*ptr_ = ptr + 20;
		}

		void LinkPageDescriptor(uint64 nextAddress, void *ptr_)
		{
			_ASSERT(ptr_);
			uint8	*ptr = static_cast<uint8 *>(ptr_);
			_ASSERT(ptr);

			*reinterpret_cast<uint64 *>(ptr + 12) = nextAddress | 2;	// [1] �͖��������
		}
	};
}
}

#endif

