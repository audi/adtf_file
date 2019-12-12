/**
 * @file
 * Disk based ring buffer
 *
 * @copyright
 * @verbatim
   Copyright @ 2017 Audi Electronics Venture GmbH. All rights reserved.

       This Source Code Form is subject to the terms of the Mozilla
       Public License, v. 2.0. If a copy of the MPL was not distributed
       with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

   If it is not possible or desirable to put the notice in a particular file, then
   You may include the notice in a location (such as a LICENSE file in a
   relevant directory) where a recipient would be likely to look for such a notice.

   You may add additional accurate notices of copyright ownership.
   @endverbatim
 */

#ifndef DISK_RING_BUFFER_H
#define DISK_RING_BUFFER_H

namespace utils5ext
{

/**
 * File-based ring buffer.
 *
 * A ring buffer implementation that supports variable sized elements and stores the actual data
 * in a file. Mind that bookkeeping data is still kept in memory.
 *
 * Use the template argument to store any additional data that you need in the drop callback which
 * is called whenever an item is removed from the head of the buffer.
 *
 * Start by calling the constructor with an already opend file. Then append items using the
 * AppendItem() method. When you want to start wrapping around, call StartWrappingAround(). This will
 * limit the ring buffer size by the current size. When you want to continue writing past the end
 * of the ringbuffer, call StartAppending() and the file will then grow indefinitly.
 */
template <typename ADDITIONAL_DATA = uint8_t, uint8_t alignment = 1>
class FileRingBuffer
{
    public:
        /**
         * This struct store information about an item in the buffer
         */
        struct Item
        {
            Item(): file_pos(-1), size(0) {}
            FilePos file_pos; ///< The file position of the items data
            size_t size; ///< The size of the items data
            ADDITIONAL_DATA additional; ///< additional bookkeeping data
        };

        /**
         * Callback interface for dropped items.
         */
        class DropCallback
        {
            public:
                /**
                 * Called whenever an item is removed from the head of the buffer.
                 * @param [in] droppedItem The dropped item
                 * @param [in] nextItem The item after the dropped item, i.e. the new head of the
                 *             buffer.
                 */
                virtual void onDrop(const Item& dropped_item, const Item& next_item) = 0;
        };

        /**
         * Helper struct to store an item that is made up by multiple data buffers.
         */
        struct ItemPiece
        {
            const void* data; ///< The data of the piece.
            size_t data_size; ///< The size of the piece.
        };

        /// Typedef for convienient access.
        typedef std::deque<Item> Items;
        /// An const iterator in the buffer.
        typedef typename Items::const_iterator const_iterator;

    private:
        Items                        _items;
        File*                        _file;
        FilePos                      _start_offset;
        FilePos                      _current_pos;
        FileSize                     _current_size;
        FileSize                     _max_size;
        bool                         _bookkeeping;
        DropCallback*                _callback;
        a_util::memory::MemoryBuffer _alignment_buffer;
        Item                         _rear_item;

    public:
        /**
         * Constructor.
         * @param [in] file A pointer to an open file.
         * @param [in] startOffset The offset where the ring buffer should start.
         * @param [in] maxSize An optional size after which wrapping around will start.
         *             automatically, if zero, use StartWrappingAround.
         * @param [in] dropCallback A callback that will inform about dropped items.
         */
        FileRingBuffer(File* file, FilePos start_offset = 0,
            FileSize max_size = 0, DropCallback* drop_callback = nullptr) :
            _file(file),
            _start_offset(start_offset),
            _current_size(0),
            _max_size(max_size),
            _bookkeeping(true),
            _callback(drop_callback)
        {
            file->setFilePos(start_offset, File::fp_begin);
            _current_pos = start_offset;

#ifdef WIN32
    __pragma(warning(push))
    __pragma(warning(disable:4127))
#endif
                if (alignment > 1)
                {
                    _alignment_buffer.allocate(alignment - 1);
                    utils5ext::memZero(_alignment_buffer.getPtr(), _alignment_buffer.getSize());
                }
#ifdef WIN32
    __pragma(warning(pop))
#endif
                fillForAlignment();
        }

        /**
         * Returns the current size of the buffer.
         * @return The current size of the buffer.
         */
        const FileSize& getCurrentSize()
        {
            return _current_size;
        }

        /**
         * Limits the size of the buffer by the current size.
         */
        void startWrappingAround()
        {
            if (!_bookkeeping)
            {
                throw std::runtime_error("history already started wrapping around");
            }

            _max_size = _current_size;
        }

        /**
         * After a call to this method, data will be appended after the ring buffer.
         * @param [out] rearDataItem A pointer that will bes set to point to the rear item in
         *              the buffer (the position where the buffer wraps around).
         * @param [out] lastDataItem A pointer that will bes set to point to the last item in
         *              the buffer.
         */
        void startAppending(Item* rear_data_item = nullptr, Item* last_data_item = nullptr)
        {
            if (!_bookkeeping)
            {
                throw std::runtime_error("already appending");
            }

            _max_size = 0;
            _file->setFilePos(0, File::fp_end);
            _current_pos = _file->getFilePos();
            _bookkeeping = false;

            if (_rear_item.file_pos == -1 && !_items.empty())
            {
                // we never wrapped around.
                _rear_item = _items.back();
            }

            if (rear_data_item)
            {
                *rear_data_item = _rear_item;
            }

            if (last_data_item)
            {
                if (!_items.empty())
                {
                    *last_data_item = _items.back();
                }
                else
                {
                    *last_data_item = Item();
                }
            }
        }

        /**
         * Adds a new item at the end of the buffer.
         * @param [in] data The data of the item.
         * @param [in] dataSize The size of the data.
         * @param [in] additional Additional bookkeeping data.
         * @param [out] filePos The postion where the item was stored.
         */
        void appendItem(const void* data, size_t data_size,
                        const ADDITIONAL_DATA& additional, FilePos* file_pos = nullptr)
        {
            ItemPiece item;
            item.data = data;
            item.dataSize = data_size;
            appendItem(&item, 1, additional, file_pos);
        }

        /**
         * Adds a new item that is made up by multiple data buffers at the end of the buffer.
         * @param [in] pieces An array of item pieces.
         * @param [in] count The amount of pieces in the array.
         * @param [in] additional Additional bookkeeping data.
         * @param [out] filePos The postion where the item was stored.
         */
        void appendItem(const ItemPiece* pieces, size_t count,
                        const ADDITIONAL_DATA& additional, FilePos* file_pos = nullptr)
        {
            size_t data_size = 0;
            for (size_t piece = 0; piece < count; ++piece)
            {
                data_size += pieces[piece].data_size;
            }

            if (_max_size && !_items.empty())
            {
                if (_current_pos + static_cast<FileSize>(data_size) > _max_size)
                {
                    // in this case we need to wrap around
                    _file->truncate(_current_pos);
                    _current_size = _current_pos;

                    _rear_item = _items.back();

                    //we need remove all items following this postion
                    while (_items.front().file_pos >= _current_pos)
                    {
                        popFront();
                    }

                    _current_pos = _items.front().file_pos; // this should be equal to the data offset
                    _file->setFilePos(_current_pos, File::fp_begin);
                }
            }

            if (file_pos)
            {
                *file_pos = _current_pos;
            }

            FilePos write_pos = _current_pos;

            for (size_t piece = 0; piece < count; ++piece)
            {
                _file->writeAll(pieces[piece].data, static_cast<int>(pieces[piece].data_size));
            }

            _current_pos += data_size;

            fillForAlignment();

            if (_current_pos > _current_size)
            {
                _current_size = _current_pos;
            }

            if (_bookkeeping)
            {
                Item item;
                item.file_pos = write_pos;
                item.size = data_size;
                item.additional = additional;

                _items.push_back(item);

                FilePos start = item.file_pos;
                FilePos end = _current_pos;

                while (_items.size() > 1 &&
                    _items.front().file_pos >= start &&
                    _items.front().file_pos < end)
                {
                    popFront();
                }

                if (_rear_item.file_pos != -1)
                {
                    // we did wrap around at least once
                    // check if this item did overwrite the current rear item
                    if (_rear_item.file_pos < end)
                    {
                        // it has been overwritten by the current one
                        // and was alread removed from the queue in the while loop above
                        // so the current item is the new rear item.
                        _rear_item = _items.back();
                        // make sure that the file ends after the current item
                        _file->truncate(_current_pos);
                        _current_size = _current_pos;
                    }
                }
            }
        }

        /**
         * Retunrs an iterator to the beginning of the buffer.
         * @return An iterator to the beginning of the buffer.
         */
        const_iterator begin() const
        {
            return _items.begin();
        }

        /**
         * Retuns an iterator to the end of the buffer.
         * @return An iterator to the end of the buffer.
         */
        const_iterator end() const
        {
            return _items.end();
        }

    protected:
        /**
         * Writes data to the file until a field position is reached that satisfies the alignment
         * requirement.
         */
        void fillForAlignment()
        {
            FilePos mod = _current_pos % alignment;
            if (mod)
            {
                FilePos fill = alignment - mod;
                _file->writeAll(_alignment_buffer.getPtr(),
                                  static_cast<size_t>(fill));
                _current_pos += fill;
            }
        }

        /**
         * Removes the first item in the buffer.
         */
        void popFront()
        {
            if (_callback)
            {
                static Item dummy;

                Item& item = _items.front();
                Item* next = &dummy;
                if (_items.size() > 1)
                {
                    next = &_items[1];
                }
                _callback->onDrop(item, *next);
            }
            _items.pop_front();
        }
};

}

#endif
