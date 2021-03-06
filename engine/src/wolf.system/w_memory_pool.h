/*
	Project			 : Wolf Engine. Copyright(c) Pooya Eimandar (http://PooyaEimandar.com) . All rights reserved.
	Source			 : Please direct any bug to https://github.com/PooyaEimandar/Wolf.Engine/issues
	Website			 : http://WolfSource.io
	Name			 : w_memory_pool based intel tbb fixed memory pool
	Description		 : Memory pool manager
	Comment          :
*/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __W_MEMORY_POOL_H__
#define __W_MEMORY_POOL_H__

#include "w_system_export.h"
#include "w_std.h"

#define __1KB__ 1024
#define __1MB__ 1024 * __1KB__
#define __1GB__ 1024 * __1MB__
#define __1TB__ 1024 * __1GB__
#define __1PB__ 1024 * __1TB__


namespace wolf
{
	namespace system
	{
		struct w_memory_pool
		{
		public:
            w_memory_pool()
            {
            }

            ~w_memory_pool()
            {
                release();
            }

            //Allocate block of memory (in bytes)
            void* alloc(_In_ size_t pSizeInBytes)
            {
                this->_size_in_bytes = pSizeInBytes;
                this->_ptr = malloc(pSizeInBytes);
                return this->_ptr;
            }

            //Re-allocate block of memory (in bytes)
            void* re_alloc(_In_ size_t pSizeInBytes)
            {
                this->_size_in_bytes = pSizeInBytes;
                this->_ptr = realloc(this->_ptr, pSizeInBytes);
                return this->_ptr;
            }
            
            //Discard all allocated memory
            ULONG release()
            {
                if (this->_is_released) return 1;

                free(this->_ptr);
                this->_is_released = true;
                return 0;
            }

            //Get pointer to start of memory
            void* get_start_ptr()
            {
                return this->_ptr;
            }

            //Get total size of memory in bytes
            const size_t get_size_in_bytes() const
            {
                return this->_size_in_bytes;
            }

        private:
            //Prevent copying
            w_memory_pool(w_memory_pool const&);
            w_memory_pool& operator= (w_memory_pool const&);

            void*                                           _ptr = nullptr;
            size_t                                          _size_in_bytes = 0;
            bool                                            _is_released = false;
		};
	}
}

#endif //__W_MEMORY_FIXED_POOL_H__
