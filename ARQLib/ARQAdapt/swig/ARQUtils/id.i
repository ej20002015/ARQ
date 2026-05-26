#pragma once

%{
#include <ARQUtils/id.h>
%}

%include <stdint.i>

namespace ARQ
{
namespace ID
{

// TYPEMAP: C# System.GUID <---> C++ ARQ::ID::UUID
%typemap(cscode) ARQ::ID::UUID %{

    // --- C++ to C# ---
    public System.Guid ToGuid()
    {
        global::System.IntPtr ptr = new global::System.IntPtr(this.getBytes());
        if (ptr == global::System.IntPtr.Zero) 
            return System.Guid.Empty;
        
        // Copy the 16 bytes from C++ memory
        byte[] b = new byte[16];
        global::System.Runtime.InteropServices.Marshal.Copy(ptr, b, 0, 16);
        
        // UUIDv7 stores bytes in Big-Endian (network byte order).
        // The C# Guid(int, short, short, byte...) constructor expects host-endian.
        // We manually reconstruct the first 3 chunks to fix the endianness instantly.
        int d1 = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
        short d2 = (short)((b[4] << 8) | b[5]);
        short d3 = (short)((b[6] << 8) | b[7]);
        
        return new System.Guid(d1, d2, d3, b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
    }

    public static implicit operator System.Guid(UUID id)
    {
        return id?.ToGuid() ?? System.Guid.Empty;
    }

    // --- C# to C++ ---
    public static implicit operator UUID(System.Guid guid)
    {
        if (guid == System.Guid.Empty) 
            return new UUID();
        
        byte[] b = guid.ToByteArray();
        
        // Revert C# Little-Endian back to UUIDv7 Big-Endian (Network Byte Order)
        byte[] v7 = new byte[16];
        v7[0] = b[3]; v7[1] = b[2]; v7[2] = b[1]; v7[3] = b[0];
        v7[4] = b[5]; v7[5] = b[4];
        v7[6] = b[7]; v7[7] = b[6];
        global::System.Array.Copy(b, 8, v7, 8, 8);

        // Create a new C++ UUID object and push the bytes into it
        UUID cppId = new UUID();
        global::System.IntPtr ptr = global::System.Runtime.InteropServices.Marshal.AllocHGlobal(16);
        try 
        {
            global::System.Runtime.InteropServices.Marshal.Copy(v7, 0, ptr, 16);
            cppId.setBytes(ptr.ToInt64());
        } 
        finally {
            global::System.Runtime.InteropServices.Marshal.FreeHGlobal(ptr); // Safe memory cleanup
        }
        
        return cppId;
    }

    public static UUID FromGuid(System.Guid guid) => guid;

    public override string ToString()
    {
        return this.ToGuid().ToString();
    }
  
    public override int GetHashCode()
    {
        return this.ToGuid().GetHashCode();
    }

%}

struct UUID
{
    static UUID create();

    %extend {
        int64_t getBytes() const
        {
            return reinterpret_cast<int64_t>( $self->bytes.data() );
        }

        void setBytes( int64_t ptr )
        {
            std::memcpy( $self->bytes.data(), reinterpret_cast<const void*>( ptr ), 16 );
        }
    }
};

}
}