/* -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 99; -*- */
/* vim: set ts=4 sw=4 et tw=99:  */
/*
    This file is part of Icecream.

    Copyright (c) 2004 Michael Matz <matz@suse.de>
                  2004 Stephan Kulow <coolo@suse.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef ICECREAM_COMM_H
#define ICECREAM_COMM_H

#ifdef __linux__
#  include <stdint.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "job.h"

// if you increase the PROTOCOL_VERSION, add a macro below and use that
#define PROTOCOL_VERSION 44
// if you increase the MIN_PROTOCOL_VERSION, comment out macros below and clean up the code
#define MIN_PROTOCOL_VERSION 21

#define MAX_SCHEDULER_PONG 3
// MAX_SCHEDULER_PING must be multiple of MAX_SCHEDULER_PONG
#define MAX_SCHEDULER_PING 12 * MAX_SCHEDULER_PONG
// maximum amount of time in seconds a daemon can be busy installing
#define MAX_BUSY_INSTALLING 120

#define IS_PROTOCOL_22(c) ((c)->protocol >= 22)
#define IS_PROTOCOL_23(c) ((c)->protocol >= 23)
#define IS_PROTOCOL_24(c) ((c)->protocol >= 24)
#define IS_PROTOCOL_25(c) ((c)->protocol >= 25)
#define IS_PROTOCOL_26(c) ((c)->protocol >= 26)
#define IS_PROTOCOL_27(c) ((c)->protocol >= 27)
#define IS_PROTOCOL_28(c) ((c)->protocol >= 28)
#define IS_PROTOCOL_29(c) ((c)->protocol >= 29)
#define IS_PROTOCOL_30(c) ((c)->protocol >= 30)
#define IS_PROTOCOL_31(c) ((c)->protocol >= 31)
#define IS_PROTOCOL_32(c) ((c)->protocol >= 32)
#define IS_PROTOCOL_33(c) ((c)->protocol >= 33)
#define IS_PROTOCOL_34(c) ((c)->protocol >= 34)
#define IS_PROTOCOL_35(c) ((c)->protocol >= 35)
#define IS_PROTOCOL_36(c) ((c)->protocol >= 36)
#define IS_PROTOCOL_37(c) ((c)->protocol >= 37)
#define IS_PROTOCOL_38(c) ((c)->protocol >= 38)
#define IS_PROTOCOL_39(c) ((c)->protocol >= 39)
#define IS_PROTOCOL_40(c) ((c)->protocol >= 40)
#define IS_PROTOCOL_41(c) ((c)->protocol >= 41)
#define IS_PROTOCOL_42(c) ((c)->protocol >= 42)
#define IS_PROTOCOL_43(c) ((c)->protocol >= 43)
#define IS_PROTOCOL_44(c) ((c)->protocol >= 44)

// Terms used:
// S  = scheduler
// C  = client
// CS = daemon

enum MsgType {
    // so far unknown
    M_UNKNOWN = 'A',

    /* When the scheduler didn't get M_STATS from a CS
       for a specified time (e.g. 10m), then he sends a
       ping */
    M_PING,

    /* Either the end of file chunks or connection (A<->A) */
    M_END,

    M_TIMEOUT, // unused

    // C --> CS
    M_GET_NATIVE_ENV,
    // CS -> C
    M_NATIVE_ENV,

    // C --> S
    M_GET_CS,
    // S --> C
    M_USE_CS,  // = 'H'
    // C --> CS
    M_COMPILE_FILE, // = 'I'
    // generic file transfer
    M_FILE_CHUNK,
    // CS --> C
    M_COMPILE_RESULT,

    // CS --> S (after the C got the CS from the S, the CS tells the S when the C asks him)
    M_JOB_BEGIN,
    M_JOB_DONE,     // = 'M'

    // C --> CS, CS --> S (forwarded from C), _and_ CS -> C as start ping
    M_JOB_LOCAL_BEGIN, // = 'N'
    M_JOB_LOCAL_DONE,

    // CS --> S, first message sent
    M_LOGIN,
    // CS --> S (periodic)
    M_STATS,

    // messages between monitor and scheduler
    M_MON_LOGIN,
    M_MON_GET_CS,
    M_MON_JOB_BEGIN, // = 'T'
    M_MON_JOB_DONE,
    M_MON_LOCAL_JOB_BEGIN,
    M_MON_STATS,

    M_TRANFER_ENV, // = 'X'

    M_TEXT,
    M_STATUS_TEXT, // = 'Z'
    M_GET_INTERNALS,

    // S --> CS, answered by M_LOGIN
    M_CS_CONF,

    // C --> CS, after installing an environment
    M_VERIFY_ENV,
    // CS --> C
    M_VERIFY_ENV_RESULT,
    // C --> CS, CS --> S (forwarded from C), to not use given host for given environment
    M_BLACKLIST_HOST_ENV,
    // S --> CS
    M_NO_CS
};

enum Compression {
    C_LZO = 0,
    C_ZSTD = 1
};

// The remote node is capable of unpacking environment compressed as .tar.xz .
const int NODE_FEATURE_ENV_XZ = ( 1 << 0 );
// The remote node is capable of unpacking environment compressed as .tar.zst .
const int NODE_FEATURE_ENV_ZSTD = ( 1 << 1 );

class MsgChannel;

// a list of pairs of host platform, filename
typedef std::list<std::pair<std::string, std::string> > Environments;

class Msg
{
public:
    Msg(enum MsgType t)
        : type(t) {}
    virtual ~Msg() {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    enum MsgType type;
};

class MsgChannel
{
public:
    enum SendFlags {
        SendBlocking = 1 << 0,
        SendNonBlocking = 1 << 1,
        SendBulkOnly = 1 << 2
    };

    virtual ~MsgChannel();

    void setBulkTransfer();

    std::string dump() const;
    // NULL  <--> channel closed or timeout
    // Will warn in log if EOF and !eofAllowed.
    Msg *get_msg(int timeout = 10, bool eofAllowed = false);

    // false <--> error (msg not send)
    bool send_msg(const Msg &, int SendFlags = SendBlocking);

    bool has_msg(void) const
    {
        return eof || instate == HAS_MSG;
    }

    // Returns ture if there were no errors filling inbuf.
    bool read_a_bit(void);

    bool at_eof(void) const
    {
        return instate != HAS_MSG && eof;
    }

    bool is_text_based(void) const
    {
        return text_based;
    }

    void readcompressed(unsigned char **buf, size_t &_uclen, size_t &_clen);
    void writecompressed(const unsigned char *in_buf,
                         size_t _in_len, size_t &_out_len);
    void write_environments(const Environments &envs);
    void read_environments(Environments &envs);
    void read_line(std::string &line);
    void write_line(const std::string &line);

    bool eq_ip(const MsgChannel &s) const;

    MsgChannel &operator>>(uint32_t &);
    MsgChannel &operator>>(std::string &);
    MsgChannel &operator>>(std::list<std::string> &);

    MsgChannel &operator<<(uint32_t);
    MsgChannel &operator<<(const std::string &);
    MsgChannel &operator<<(const std::list<std::string> &);

    // our filedesc
    int fd;

    // the minimum protocol version between me and him
    int protocol;
    // the actual maximum protocol the remote supports
    int maximum_remote_protocol;

    std::string name;
    time_t last_talk;

protected:
    MsgChannel(int _fd, struct sockaddr *, socklen_t, bool text = false);

    bool wait_for_protocol();
    // returns false if there was an error sending something
    bool flush_writebuf(bool blocking);
    void writefull(const void *_buf, size_t count);
    // returns false if there was an error in the protocol setup
    bool update_state(void);
    void chop_input(void);
    void chop_output(void);
    bool wait_for_msg(int timeout);
    void set_error(bool silent = false);

    char *msgbuf;
    size_t msgbuflen;
    size_t msgofs;
    size_t msgtogo;
    char *inbuf;
    size_t inbuflen;
    size_t inofs;
    size_t intogo;

    enum {
        NEED_PROTO,
        NEED_LEN,
        FILL_BUF,
        HAS_MSG,
        ERROR
    } instate;

    uint32_t inmsglen;
    bool eof;
    bool text_based;

private:
    friend class Service;

    // deep copied
    struct sockaddr *addr;
    socklen_t addr_len;
    bool set_error_recursion;
};

// just convenient functions to create MsgChannels
class Service
{
public:
    static MsgChannel *createChannel(const std::string &host, unsigned short p, int timeout);
    static MsgChannel *createChannel(const std::string &domain_socket);
    static MsgChannel *createChannel(int remote_fd, struct sockaddr *, socklen_t);
};

class Broadcasts
{
public:
    // Broadcasts a message about this scheduler and its information.
    static void broadcastSchedulerVersion(int scheduler_port, const char* netname, time_t starttime);
    // Checks if the data received is a scheduler version broadcast.
    static bool isSchedulerVersion(const char* buf, int buflen);
    // Reads data from a scheduler version broadcast.
    static void getSchedulerVersionData( const char* buf, int* protocol, time_t* time, std::string* netname );
    /// Broadcasts the given data on the given port.
    static const int BROAD_BUFLEN = 268;
private:
    static void broadcastData(int port, const char* buf, int size);
};

// --------------------------------------------------------------------------
// this class is also used by icecream-monitor
class DiscoverSched
{
public:
    /* Connect to a scheduler waiting max. TIMEOUT seconds.
       schedname can be the hostname of a box running a scheduler, to avoid
       broadcasting, port can be specified explicitly */
    DiscoverSched(const std::string &_netname = std::string(),
                  int _timeout = 2,
                  const std::string &_schedname = std::string(),
                  int port = 0);
    ~DiscoverSched();

    bool timed_out();

    int listen_fd() const
    {
        return schedname.empty() ? ask_fd : -1;
    }

    int connect_fd() const
    {
        return schedname.empty() ? -1 : ask_fd;
    }

    // compat for icecream monitor
    int get_fd() const
    {
        return listen_fd();
    }

    /* Attempt to get a conenction to the scheduler.
    
       Continue to call this while it returns NULL and timed_out() 
       returns false. If this returns NULL you should wait for either
       more data on listen_fd() (use select), or a timeout of your own.  
       */
    MsgChannel *try_get_scheduler();

    // Returns the hostname of the scheduler - set by constructor or by try_get_scheduler
    std::string schedulerName() const
    {
        return schedname;
    }

    // Returns the network name of the scheduler - set by constructor or by try_get_scheduler
    std::string networkName() const
    {
        return netname;
    }

    /* Return a list of all reachable netnames.  We wait max. WAITTIME
       milliseconds for answers.  */
    static std::list<std::string> getNetnames(int waittime = 2000, int port = 8765);

    // Checks if the data is from a scheduler discovery broadcast, returns version of the sending
    // daemon is yes.
    static bool isSchedulerDiscovery(const char* buf, int buflen, int* daemon_version);
    // Prepares data for sending a reply to a scheduler discovery broadcast.
    static int prepareBroadcastReply(char* buf, const char* netname, time_t starttime);

private:
    struct sockaddr_in remote_addr;
    std::string netname;
    std::string schedname;
    int timeout;
    int ask_fd;
    int ask_second_fd; // for debugging
    time_t time0;
    unsigned int sport;
    int best_version;
    time_t best_start_time;
    std::string best_schedname;
    int best_port;
    bool multiple;

    void attempt_scheduler_connect();
    void sendSchedulerDiscovery( int version );
    static bool get_broad_answer(int ask_fd, int timeout, char *buf2, struct sockaddr_in *remote_addr,
                 socklen_t *remote_len);
    static void get_broad_data(const char* buf, const char** name, int* version, time_t* start_time);
};
// --------------------------------------------------------------------------

/* Return a list of all reachable netnames.  We wait max. WAITTIME
   milliseconds for answers.  */
std::list<std::string> get_netnames(int waittime = 2000, int port = 8765);

class PingMsg : public Msg
{
public:
    PingMsg()
        : Msg(M_PING) {}
};

class EndMsg : public Msg
{
public:
    EndMsg()
        : Msg(M_END) {}
};

class GetCSMsg : public Msg
{
public:
    GetCSMsg()
        : Msg(M_GET_CS)
        , count(1)
        , arg_flags(0)
        , client_id(0)
        , client_count(0)
        , niceness(0)
        {}

    GetCSMsg(const Environments &envs, const std::string &f,
             CompileJob::Language _lang, unsigned int _count,
             std::string _target, unsigned int _arg_flags,
             const std::string &host, int _minimal_host_version,
             unsigned int _required_features,
             int _niceness,
             unsigned int _client_count = 0);

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    Environments versions;
    std::string filename;
    CompileJob::Language lang;
    uint32_t count; // the number of UseCS messages to answer with - usually 1
    std::string target;
    uint32_t arg_flags;
    uint32_t client_id;
    std::string preferred_host;
    int minimal_host_version;
    uint32_t required_features;
    uint32_t client_count; // number of CS -> C connections at the moment
    uint32_t niceness; // nice priority (0-20)
};

class UseCSMsg : public Msg
{
public:
    UseCSMsg()
        : Msg(M_USE_CS) {}
    UseCSMsg(std::string platform, std::string host, unsigned int p, unsigned int id, bool gotit,
             unsigned int _client_id, unsigned int matched_host_jobs)
        : Msg(M_USE_CS),
          job_id(id),
          hostname(host),
          port(p),
          host_platform(platform),
          got_env(gotit),
          client_id(_client_id),
          matched_job_id(matched_host_jobs) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    uint32_t job_id;
    std::string hostname;
    uint32_t port;
    std::string host_platform;
    uint32_t got_env;
    uint32_t client_id;
    uint32_t matched_job_id;
};

class NoCSMsg : public Msg
{
public:
    NoCSMsg()
        : Msg(M_NO_CS) {}
    NoCSMsg(unsigned int id, unsigned int _client_id)
        : Msg(M_NO_CS),
          job_id(id),
          client_id(_client_id) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    uint32_t job_id;
    uint32_t client_id;
};

class GetNativeEnvMsg : public Msg
{
public:
    GetNativeEnvMsg()
        : Msg(M_GET_NATIVE_ENV) {}

    GetNativeEnvMsg(const std::string &c, const std::list<std::string> &e,
        const std::string &comp)
        : Msg(M_GET_NATIVE_ENV)
        , compiler(c)
        , extrafiles(e)
        , compression(comp)
        {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    std::string compiler; // "gcc", "clang" or the actual binary
    std::list<std::string> extrafiles;
    std::string compression; // "" (=default), "none", "gzip", "xz", etc.
};

class UseNativeEnvMsg : public Msg
{
public:
    UseNativeEnvMsg()
        : Msg(M_NATIVE_ENV) {}

    UseNativeEnvMsg(std::string _native)
        : Msg(M_NATIVE_ENV)
        , nativeVersion(_native) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    std::string nativeVersion;
};

class CompileFileMsg : public Msg
{
public:
    CompileFileMsg(CompileJob *j, bool delete_job = false)
        : Msg(M_COMPILE_FILE)
        , deleteit(delete_job)
        , job(j) {}

    ~CompileFileMsg()
    {
        if (deleteit) {
            delete job;
        }
    }

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;
    CompileJob *takeJob();

private:
    std::string remote_compiler_name() const;

    bool deleteit;
    CompileJob *job;
};

class FileChunkMsg : public Msg
{
public:
    FileChunkMsg(unsigned char *_buffer, size_t _len)
        : Msg(M_FILE_CHUNK)
        , buffer(_buffer)
        , len(_len)
        , del_buf(false) {}

    FileChunkMsg()
        : Msg(M_FILE_CHUNK)
        , buffer(0)
        , len(0)
        , del_buf(true) {}

    ~FileChunkMsg();

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    unsigned char *buffer;
    size_t len;
    mutable size_t compressed;
    bool del_buf;

private:
    FileChunkMsg(const FileChunkMsg &);
    FileChunkMsg &operator=(const FileChunkMsg &);
};

class CompileResultMsg : public Msg
{
public:
    CompileResultMsg()
        : Msg(M_COMPILE_RESULT)
        , status(0)
        , was_out_of_memory(false)
        , have_dwo_file(false) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    int status;
    std::string out;
    std::string err;
    bool was_out_of_memory;
    bool have_dwo_file;
};

class JobBeginMsg : public Msg
{
public:
    JobBeginMsg()
        : Msg(M_JOB_BEGIN)
        , client_count(0) {}

    JobBeginMsg(unsigned int j, unsigned int _client_count)
        : Msg(M_JOB_BEGIN)
        , job_id(j)
        , stime(time(0))
        , client_count(_client_count) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    uint32_t job_id;
    uint32_t stime;
    uint32_t client_count; // number of CS -> C connections at the moment
};

class JobDoneMsg : public Msg
{
public:
    /* FROM_SERVER: this message was generated by the daemon responsible
          for remotely compiling the job (i.e. job->server).
       FROM_SUBMITTER: this message was generated by the daemon connected
          to the submitting client.  */
    enum from_type {
        FROM_SERVER = 0,
        FROM_SUBMITTER = 1
    };

    // other flags
    enum {
        UnknownJobId = (1 << 1)
    };

    JobDoneMsg(int job_id = 0, int exitcode = -1, unsigned int flags = FROM_SERVER,
               unsigned int _client_count = 0);

    void set_from(from_type from)
    {
        flags |= (uint32_t)from;
    }

    bool is_from_server()
    {
        return (flags & FROM_SUBMITTER) == 0;
    }

    void set_unknown_job_client_id( uint32_t clientId );
    uint32_t unknown_job_client_id() const;
    void set_job_id( uint32_t jobId );

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    uint32_t real_msec; /* real time it used */
    uint32_t user_msec; /* user time used */
    uint32_t sys_msec; /* system time used */
    uint32_t pfaults; /* page faults */

    int exitcode; /* exit code */

    uint32_t flags;

    uint32_t in_compressed;
    uint32_t in_uncompressed;
    uint32_t out_compressed;
    uint32_t out_uncompressed;

    uint32_t job_id;
    uint32_t client_count; // number of CS -> C connections at the moment
};

class JobLocalBeginMsg : public Msg
{
public:
    JobLocalBeginMsg(int job_id = 0, const std::string &file = "", bool full = false)
        : Msg(M_JOB_LOCAL_BEGIN)
        , outfile(file)
        , stime(time(0))
        , id(job_id)
        , fulljob(full) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    std::string outfile;
    uint32_t stime;
    uint32_t id;
    bool fulljob;
};

class JobLocalDoneMsg : public Msg
{
public:
    JobLocalDoneMsg(unsigned int id = 0)
        : Msg(M_JOB_LOCAL_DONE)
        , job_id(id) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    uint32_t job_id;
};

class LoginMsg : public Msg
{
public:
    LoginMsg(unsigned int myport, const std::string &_nodename, const std::string &_host_platform,
             unsigned int my_features);
    LoginMsg()
        : Msg(M_LOGIN)
        , port(0) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    uint32_t port;
    Environments envs;
    uint32_t max_kids;
    bool noremote;
    bool chroot_possible;
    std::string nodename;
    std::string host_platform;
    uint32_t supported_features; // bitmask of various features the node supports
};

class ConfCSMsg : public Msg
{
public:
    ConfCSMsg()
        : Msg(M_CS_CONF)
        , max_scheduler_pong(MAX_SCHEDULER_PONG)
        , max_scheduler_ping(MAX_SCHEDULER_PING) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    uint32_t max_scheduler_pong;
    uint32_t max_scheduler_ping;
};

class StatsMsg : public Msg
{
public:
    StatsMsg()
        : Msg(M_STATS)
        , load(0)
        , client_count(0)
    {
    }

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    /**
     * For now the only load measure we have is the
     * load from 0-1000.
     * This is defined to be a daemon defined value
     * on how busy the machine is. The higher the load
     * is, the slower a given job will compile (preferably
     * linear scale). Load of 1000 means to not schedule
     * another job under no circumstances.
     */
    uint32_t load;

    uint32_t loadAvg1;
    uint32_t loadAvg5;
    uint32_t loadAvg10;
    uint32_t freeMem;

    uint32_t client_count; // number of CS -> C connections at the moment
};

class EnvTransferMsg : public Msg
{
public:
    EnvTransferMsg()
        : Msg(M_TRANFER_ENV) {}

    EnvTransferMsg(const std::string &_target, const std::string &_name)
        : Msg(M_TRANFER_ENV)
        , name(_name)
        , target(_target) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    std::string name;
    std::string target;
};

class GetInternalStatus : public Msg
{
public:
    GetInternalStatus()
        : Msg(M_GET_INTERNALS) {}

    GetInternalStatus(const GetInternalStatus &)
        : Msg(M_GET_INTERNALS) {}
};

class MonLoginMsg : public Msg
{
public:
    MonLoginMsg()
        : Msg(M_MON_LOGIN) {}
};

class MonGetCSMsg : public GetCSMsg
{
public:
    MonGetCSMsg()
        : GetCSMsg()
    { // overwrite
        type = M_MON_GET_CS;
        clientid = job_id = 0;
    }

    MonGetCSMsg(int jobid, int hostid, GetCSMsg *m)
        : GetCSMsg(Environments(), m->filename, m->lang, 1, m->target, 0, std::string(), false, m->client_count, m->niceness)
        , job_id(jobid)
        , clientid(hostid)
    {
        type = M_MON_GET_CS;
    }

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    uint32_t job_id;
    uint32_t clientid;
};

class MonJobBeginMsg : public Msg
{
public:
    MonJobBeginMsg()
        : Msg(M_MON_JOB_BEGIN)
        , job_id(0)
        , stime(0)
        , hostid(0) {}

    MonJobBeginMsg(unsigned int id, unsigned int time, int _hostid)
        : Msg(M_MON_JOB_BEGIN)
        , job_id(id)
        , stime(time)
        , hostid(_hostid) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    uint32_t job_id;
    uint32_t stime;
    uint32_t hostid;
};

class MonJobDoneMsg : public JobDoneMsg
{
public:
    MonJobDoneMsg()
        : JobDoneMsg()
    {
        type = M_MON_JOB_DONE;
    }

    MonJobDoneMsg(const JobDoneMsg &o)
        : JobDoneMsg(o)
    {
        type = M_MON_JOB_DONE;
    }
};

class MonLocalJobBeginMsg : public Msg
{
public:
    MonLocalJobBeginMsg()
        : Msg(M_MON_LOCAL_JOB_BEGIN) {}

    MonLocalJobBeginMsg(unsigned int id, const std::string &_file, unsigned int time, int _hostid)
        : Msg(M_MON_LOCAL_JOB_BEGIN)
        , job_id(id)
        , stime(time)
        , hostid(_hostid)
        , file(_file) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    uint32_t job_id;
    uint32_t stime;
    uint32_t hostid;
    std::string file;
};

class MonStatsMsg : public Msg
{
public:
    MonStatsMsg()
        : Msg(M_MON_STATS) {}

    MonStatsMsg(int id, const std::string &_statmsg)
        : Msg(M_MON_STATS)
        , hostid(id)
        , statmsg(_statmsg) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    uint32_t hostid;
    std::string statmsg;
};

class TextMsg : public Msg
{
public:
    TextMsg()
        : Msg(M_TEXT) {}

    TextMsg(const std::string &_text)
        : Msg(M_TEXT)
        , text(_text) {}

    TextMsg(const TextMsg &m)
        : Msg(M_TEXT)
        , text(m.text) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    std::string text;
};

class StatusTextMsg : public Msg
{
public:
    StatusTextMsg()
        : Msg(M_STATUS_TEXT) {}

    StatusTextMsg(const std::string &_text)
        : Msg(M_STATUS_TEXT)
        , text(_text) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    std::string text;
};

class VerifyEnvMsg : public Msg
{
public:
    VerifyEnvMsg()
        : Msg(M_VERIFY_ENV) {}

    VerifyEnvMsg(const std::string &_target, const std::string &_environment)
        : Msg(M_VERIFY_ENV)
        , environment(_environment)
        , target(_target) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    std::string environment;
    std::string target;
};

class VerifyEnvResultMsg : public Msg
{
public:
    VerifyEnvResultMsg()
        : Msg(M_VERIFY_ENV_RESULT) {}

    VerifyEnvResultMsg(bool _ok)
        : Msg(M_VERIFY_ENV_RESULT)
        , ok(_ok) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    bool ok;
};

class BlacklistHostEnvMsg : public Msg
{
public:
    BlacklistHostEnvMsg()
        : Msg(M_BLACKLIST_HOST_ENV) {}

    BlacklistHostEnvMsg(const std::string &_target, const std::string &_environment, const std::string &_hostname)
        : Msg(M_BLACKLIST_HOST_ENV)
        , environment(_environment)
        , target(_target)
        , hostname(_hostname) {}

    virtual void fill_from_channel(MsgChannel *c);
    virtual void send_to_channel(MsgChannel *c) const;

    std::string environment;
    std::string target;
    std::string hostname;
};

#endif
