#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define UNIX_PATH_MAX 108

int send_fd(int socket, int fd_to_send) {
    struct msghdr socket_message;
    struct iovec io_vector[1];
    struct cmsghdr *control_message = NULL;
    char message_buffer[1];
    /* storage space needed for an ancillary element with a paylod of length is CMSG_SPACE(sizeof(length)) */
    char ancillary_element_buffer[CMSG_SPACE(sizeof(int))];
    int available_ancillary_element_buffer_space;

    /* at least one vector of one byte must be sent */
    message_buffer[0] = 'F';
    io_vector[0].iov_base = message_buffer;
    io_vector[0].iov_len = 1;

    /* initialize socket message */
    memset(&socket_message, 0, sizeof(struct msghdr));
    socket_message.msg_iov = io_vector;
    socket_message.msg_iovlen = 1;

    /* provide space for the ancillary data */
    available_ancillary_element_buffer_space = CMSG_SPACE(sizeof(int));
    memset(ancillary_element_buffer, 0, available_ancillary_element_buffer_space);
    socket_message.msg_control = ancillary_element_buffer;
    socket_message.msg_controllen = available_ancillary_element_buffer_space;

    /* initialize a single ancillary data element for fd passing */
    control_message = CMSG_FIRSTHDR(&socket_message);
    control_message->cmsg_level = SOL_SOCKET;
    control_message->cmsg_type = SCM_RIGHTS;
    control_message->cmsg_len = CMSG_LEN(sizeof(int));
    *((int *) CMSG_DATA(control_message)) = fd_to_send;

    return sendmsg(socket, &socket_message, 0);
}

int main(int argc, char *argv[]) {
    struct sockaddr_un address;
    int socket_fd, connection_fd;
    socklen_t address_length;
    pid_t child;
    char *socket_path = "/tmp/demo_socket";
    char *file_path = "";
    
    if (argc < 2) {
        printf("Usage: server <file> [socket_path]\n");
        return 1;
    }

    if (argc > 2) {
        socket_path = argv[2];
    }
    file_path = argv[1];

    socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(socket_fd < 0)
    {
        printf("socket() failed\n");
        return 1;
    } 

    unlink(socket_path);

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, UNIX_PATH_MAX, socket_path);

    int return_code = bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un));
    if(return_code != 0)
    {
        perror("bind() failed");
        return 1;
    }

    if (chmod(socket_path,  S_IRUSR |  S_IWUSR |  S_IRGRP |  S_IWGRP |  S_IROTH | S_IWOTH) != 0) {
        perror("chmod() failed");
        return 1;
    }

    if(listen(socket_fd, 5) != 0)
    {
        printf("listen() failed\n");
        return 1;
    }

    while((connection_fd = accept(socket_fd, 
                    (struct sockaddr *) &address,
                    &address_length)) > -1) {
        child = fork();
        if(child == 0)
        {
            int fid = open(file_path, O_RDONLY);
            if (fid < 0) {
                perror("Open file failed");
                return 1;
            }
            /* now inside newly created connection handling process */
            return send_fd(connection_fd, fid);
        }

        /* still inside server process */
        close(connection_fd);
    }

    close(socket_fd);
    unlink(socket_path);
    return 0;
}
