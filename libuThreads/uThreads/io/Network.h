/*******************************************************************************
 *     Copyright © 2015, 2016 Saman Barghi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

#ifndef UTHREADS_INCLUDE_NETWORK_H_
#define UTHREADS_INCLUDE_NETWORK_H_
#include "IOHandler.h"

/**
 * @class Connection
 * @brief Represents a network connection
 *
 * Connection class is a wrapper around socket and
 * provides the ability to do nonblocking read/write on sockets, and nonblocking
 * accept. It first tries to read/write/accept and if the fd is not ready
 * uses the underlying polling structure to wait for the fd to be ready. Thus,
 * the uThread that is calling these functions is blocked if the fd is not
 * ready, and kThread never blocks.
 */
class Connection {
    friend IOHandler;
private:
    /* used whith polling */
    PollData* pd = nullptr;
    //related file descriptor
    int fd = -1;

    /**
     * @brief Initialize the Connection
     * @param fd the File Descriptor
     * @param poll Whether to add this to the underlying polling structure or not
     */
    void init();

    void setFD(int fd) {
        this->fd = fd;
        this->pd->fd = fd;
    }

public:

    /**
     * @brief Create a Connection that does not have
     *
     * This is useful for accept or socket functions that require
     * a Connection object without fd being set
     */
    Connection() :
            fd(-1){
        init();
    }
    /**
     * @brief Create a connection object with the provided fd
     * @param fd
     *
     * If the connection is already established by other means, set the
     * fd and add it to the polling structure
     */
    Connection(int fd) :
            fd(fd) {
        init();
    }

    /**
     * @brief Same as socket syscall adds | SOCK_NONBLOCK to type
     * @return same as socket syscall
     *
     * Throws a std::system_error exception. Do not call from C code.
     * The unerlying socket is always nonbelocking. This is achieved
     * by adding a  (| SOCK_NONBLOCK) to type, thus requires
     * linux kernels > 2.6.27
     *
     */
    Connection(int domain, int type, int protocol) throw(std::system_error);

    ~Connection();

    /**
     * @brief nonblocking accept syscall and updating the passed Connection
     * object
     * @param conn Pointer to a Connection object that is not initialized
     * @return same as accept system call
     *
     *  This format is used for compatibility with C
     */
    int accept(Connection *conn, struct sockaddr *addr, socklen_t *addrlen);

    /**
     * @brief Accepts a connection and returns a connection object
     * @return Newly created connection
     *
     * Throws a std::system_error exception on error. Never call from C.
     */
    Connection* accept(struct sockaddr *addr, socklen_t *addrlen) throw(std::system_error);

    /**
     * @brief Same as socket syscall, set the fd for current connection
     * @return same as socket syscall
     * The unerlying socket is always nonbelocking. This is achieved
     * by adding a  (| SOCK_NONBLOCK) to type, thus requires
     * linux kernels > 2.6.27

     */
    int socket(int domain, int type, int protocol);

    /**
     * @brief Same as listen syscall on current fd
     * @return Same as listen syscall
     */
    int listen(int backlog);

    /**
     * @brief Same as bind syscall
     * @return Same as bind syscall
     */
    int bind(const struct sockaddr *addr,
             socklen_t addrlen);

    /**
     * @brief Same as connect syscall
     * @return Same as connect syscall
     */
    int connect(const struct sockaddr *addr,socklen_t addrlen);
    /**
     * @brief Call the underlying system call on Connection's file descriptor
     * @return Same as what the related systemcall returns
     *
     * This function calls the system call with the same name. If the socket is
     * ready for the required function it returns immediately, otherwise it
     * blocks in the user-level (blocks uThread not kThread), and polls the
     * file descriptor until it becomes ready.
     *
     * The return results is the same as the underlying system call except that
     * the following condition is never true when the function returns:
     *  (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)
     *
     *  which means the Connection object does the polling and only returns
     *  when an error occurs or the socket is ready.
     */
    ssize_t recv(void *buf, size_t len, int flags);
    /// @copydoc recv
    ssize_t recvfrom(void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    /// @copydoc recv
    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
    /// @copydoc recv
    int recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
                 unsigned int flags, struct timespec *timeout);

    /// @copydoc recv
    ssize_t send(const void *buf, size_t len, int flags);
    /// @copydoc recv
    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
            const struct sockaddr *dest_addr, socklen_t addrlen);
    /// @copydoc recv
    ssize_t sendmsg(const struct msghdr *msg, int flags);
    /// @copydoc recv
    int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
                 unsigned int flags);

    /// @copydoc recv
    ssize_t read(void *buf, size_t count);
    /// @copydoc recv
    ssize_t write(const void *buf, size_t count);

    /**
     * @brief Block uThread, waiting for fd to become ready for read
     */
    void blockOnRead();

    /**
     * @brief Block uThread, waiting for fd to become ready for write
     * */
    void blockOnWrite();
    /**
     * @brief closes the socket
     * @return the same as close system call
     */
    int close();

    /**
     * @brief return the Connection's file descriptor
     * @return file descriptor
     */
    int getFd() const {return fd;}

};

#endif /* UTHREADS_INCLUDE_NETWORK_H_ */
