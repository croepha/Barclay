// Copyright 2019 David Butler <croepha@gmail.com>



// TODO:20190519.01: Possibly look into doing abstract, non-pathed sockets...
#define SERVER_SOCKET_PATH  "/tmp/test_display_server.sock"


#if 1


static const int CLIENT_PROTOCOL_VERSION = 1;
struct Packet_Header {
    s32 version;
    V2<s16> mouse_pos;
    bool    did_resize;
    bool    has_focus;
    u32     text_len;
    u32     scancode_len;
};

struct ClientToServerPacket {
    bool wait_for_input;
};

#endif

struct Packet_Size {
    V2<s16> size;
};



struct ClientBitmapSpec {
    s32 w;
    s32 h;
    
    static const s32 bytes_per_px            = 4;
    inline s32 pitch              (){ return w * bytes_per_px; }
    inline size_t single_buffer_bytes(){ return (u64)(pitch() * h); }
    inline size_t double_buffer_bytes(){ return single_buffer_bytes() * 2; }
};



