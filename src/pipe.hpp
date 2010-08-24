/*
    Copyright (c) 2007-2010 iMatix Corporation

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the Lesser GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ZMQ_PIPE_HPP_INCLUDED__
#define __ZMQ_PIPE_HPP_INCLUDED__

#include "../include/zmq.h"

#include "stdint.hpp"
#include "yarray_item.hpp"
#include "ypipe.hpp"
#include "msg_store.hpp"
#include "config.hpp"
#include "object.hpp"

namespace zmq
{

    //  Creates a pipe. Returns pointer to reader and writer objects.
    void create_pipe (object_t *reader_parent_, object_t *writer_parent_,
        uint64_t hwm_, int64_t swap_size_, class reader_t **reader_,
        class writer_t **writer_);

    //  The shutdown mechanism for pipe works as follows: Either endpoint
    //  (or even both of them) can ask pipe to terminate by calling 'terminate'
    //  method. Pipe then terminates in asynchronous manner. When the part of
    //  the shutdown tied to the endpoint is done it triggers 'terminated'
    //  event. When endpoint processes the event and returns, associated
    //  reader/writer object is deallocated.

    typedef ypipe_t <zmq_msg_t, message_pipe_granularity> pipe_t;

    struct i_reader_events
    {
        virtual void terminated (class reader_t *pipe_) = 0;
        virtual void activated (class reader_t *pipe_) = 0;
    };

    class reader_t : public object_t, public yarray_item_t
    {
        friend void zmq::create_pipe (object_t*, object_t*, uint64_t,
            int64_t, reader_t**, writer_t**);
        friend class writer_t;

    public:

        //  Specifies the object to get events from the reader.
        void set_event_sink (i_reader_events *endpoint_);

        //  Returns true if there is at least one message to read in the pipe.
        bool check_read ();

        //  Reads a message to the underlying pipe.
        bool read (zmq_msg_t *msg_);

        //  Ask pipe to terminate.
        void terminate ();

    private:

        reader_t (class object_t *parent_, pipe_t *pipe_, uint64_t lwm_);
        ~reader_t ();

        //  To be called only by writer itself!
        void set_writer (class writer_t *writer_);

        //  Command handlers.
        void process_revive ();
        void process_pipe_term_ack ();

        //  Returns true if the message is delimiter; false otherwise.
        static bool is_delimiter (zmq_msg_t &msg_);

        //  The underlying pipe.
        pipe_t *pipe;

        //  Pipe writer associated with the other side of the pipe.
        class writer_t *writer;

        //  Low watermark for in-memory storage (in bytes).
        uint64_t lwm;

        //  Number of messages read so far.
        uint64_t msgs_read;

        //  Sink for the events (either the socket of the session).
        i_reader_events *sink;

        //  True is 'terminate' method was called or delimiter
        //  was read from the pipe.
        bool terminating;

        reader_t (const reader_t&);
        void operator = (const reader_t&);
    };

    struct i_writer_events
    {
        virtual void terminated (class writer_t *pipe_) = 0;
        virtual void activated (class writer_t *pipe_) = 0;
    };

    class writer_t : public object_t, public yarray_item_t
    {
        friend void zmq::create_pipe (object_t*, object_t*, uint64_t,
            int64_t, reader_t**, writer_t**);

    public:

        //  Specifies the object to get events from the writer.
        void set_event_sink (i_writer_events *endpoint_);

        //  Checks whether a message can be written to the pipe.
        //  If writing the message would cause high watermark to be
        //  exceeded, the function returns false.
        bool check_write ();

        //  Writes a message to the underlying pipe. Returns false if the
        //  message cannot be written because high watermark was reached.
        bool write (zmq_msg_t *msg_);

        //  Remove unfinished part of a message from the pipe.
        void rollback ();

        //  Flush the messages downsteam.
        void flush ();

        //  Ask pipe to terminate.
        void terminate ();

    private:

        writer_t (class object_t *parent_, pipe_t *pipe_, reader_t *reader_,
            uint64_t hwm_, int64_t swap_size_);
        ~writer_t ();

        void process_reader_info (uint64_t msgs_read_);

        //  Command handlers.
        void process_pipe_term ();

        //  Tests whether the pipe is already full.
        bool pipe_full ();

        //  Write special message to the pipe so that the reader
        //  can find out we are finished.
        void write_delimiter ();

        //  The underlying pipe.
        pipe_t *pipe;

        //  Pipe reader associated with the other side of the pipe.
        reader_t *reader;

        //  High watermark for in-memory storage (in bytes).
        uint64_t hwm;

        //  Last confirmed number of messages read from the pipe.
        //  The actual number can be higher.
        uint64_t msgs_read;

        //  Number of messages we have written so far.
        uint64_t msgs_written;

        //  Pointer to backing store. If NULL, messages are always
        //  kept in main memory.
        msg_store_t *msg_store;

        bool extra_msg_flag;

        zmq_msg_t extra_msg;

        //  True iff the last attempt to write a message has failed.
        bool stalled;

        //  Sink for the events (either the socket or the session).
        i_writer_events *sink;

        //  True is 'terminate' method was called of 'pipe_term' command
        //  arrived from the reader.
        bool terminating;

        bool pending_close;

        writer_t (const writer_t&);
        void operator = (const writer_t&);
    };

}

#endif
