#pragma once

namespace purecpp{
    enum ErrorCode{
        no_error,
        io_error,
        redis_parse_error,  //redis protocal parse error
        redis_reject_error, //rejected by redis server
        unknown_error
    };
}