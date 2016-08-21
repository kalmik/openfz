//
// Created by sergio filipe on 8/21/16.
//

#ifndef OPENFZ_REQUEST_H
#define OPENFZ_REQUEST_H

#define REQ_BUFFER_SIZE 2048

struct request_payload {
    int status;
    char msg[REQ_BUFFER_SIZE];
    void* extra;
};

#endif //OPENFZ_REQUEST_H
