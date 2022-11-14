#include <fstream>
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <functional>
#include "stringconsts.h"
#include <arpa/inet.h> // ntohs, ntohl

// https://rpm-software-management.github.io/rpm/manual/format.html

template<typename T>
struct flipper;

template<>
struct flipper<unsigned char>
{
    static unsigned char h(unsigned char ch) { return ch; }
};
template<>
struct flipper<char>
{
    static char h(char ch) { return ch; }
};

template<>
struct flipper<short>
{
    static short h(short v) { return ntohs(v); }
};

template<>
struct flipper<int>
{
    static int h(int v) { return ntohl(v); }
};

template<typename S, typename T>
void read_int(S& s, T& v)
{
    T tmp;
    typedef typename S::char_type ch_t;
    ch_t* p = reinterpret_cast<ch_t*>(&tmp);
    s.read(p, sizeof(v));
    v = flipper<T>::h(tmp);
}

template<typename S>
void read_buf(S& s, std::vector<char>& v, size_t cnt)
{
    v.clear();
    v.resize(cnt);
    s.read(&v[0], cnt);
}

char ch2hex(unsigned char v)
{
    if (v>=0 && v<10)
        return char(v) + '0';
    if (v>=10 && v<16)
        return char(v) + 'A' - 10;
    return '.';
}

template<typename CH, typename TR>
void outhex(std::basic_ostream<CH,TR>& os, unsigned char v)
{
    os << "\\x";
    os << ch2hex((v&0xF0)>>4) << ch2hex(v&0x0F);
}

template<typename CH, typename TR>
void outcombi(std::basic_ostream<CH,TR>& os, unsigned char v)
{
    if (v<32)
        os << '.';
    else
        os << v;
    os << '(';
    outhex(os, v);
    os << ')';
}

struct lead_t
{
    static const size_t MagicSize = 4;
    unsigned char Magic[MagicSize];
    unsigned char Major, Minor;
    short Type;
    std::string getPackageType() const { switch (Type) { case 0: return "binary"; case 1: return "source"; default: return "unknown"; } }
    short ArchNum;
    static const size_t NameSize = 66;
    char Name[NameSize];
    short OsNum;
    short SignatureType;
    static const size_t ReservedSize = 16;
    unsigned char Reserved[ReservedSize];
    template<typename IS>
    void read(IS& is)
    {
        for (size_t i=0; i<MagicSize; ++i)
            read_int(is, Magic[i]);
        read_int(is, Major);
        read_int(is, Minor);
        read_int(is, Type);
        read_int(is, ArchNum);
        for (size_t i=0; i<NameSize; ++i)
            read_int(is, Name[i]);
        read_int(is, OsNum);
        read_int(is, SignatureType);
        for (size_t i=0; i<ReservedSize; ++i)
            read_int(is, Reserved[i]);
    }
};

struct header_entry_t
{
    static const size_t MagicSize = 3;
    unsigned char Magic[MagicSize];
    unsigned char Version;
    static const size_t ReservedSize = 4;
    unsigned char Reserved[ReservedSize];
    int32_t Count, Bytes;

    template<typename IS>
    void read(IS& is)
    {
        for (size_t i=0; i<MagicSize; ++i)
            read_int(is, Magic[i]);
        read_int(is, Version);
        for (size_t i=0; i<ReservedSize; ++i)
            read_int(is, Reserved[i]);
        read_int(is, Count);
        read_int(is, Bytes);
    }
};

struct index_entry_t
{
    int32_t Tag, Type, Offset, Count;

    template<typename IS>
    void read(IS& is)
    {
        read_int(is, Tag);
        read_int(is, Type);
        read_int(is, Offset);
        read_int(is, Count);
    }

    template<typename OS>
    void dump(OS& os, const std::vector<char>& buf) const
    {
        switch (Type)
        {
            case 3 /*int16*/:
            {
                if (Count > 1) os << "[ ";
                for (size_t i=0; i<Count; ++i)
                {
                    const int16_t* p = reinterpret_cast<const int16_t*>(&(buf[0]) + Offset + i);
                    os << flipper<int16_t>::h(*p);
                    if (i+1 < Count) os << ", ";
                }
                if (Count > 1) os << " ]";
                break;
            }
            case 4 /*int32*/:
            {
                if (Count > 1) os << "[ ";
                for (size_t i=0; i<Count; ++i)
                {
                    const int32_t* p = reinterpret_cast<const int32_t*>(&(buf[0]) + Offset + i);
                    os << flipper<int32_t>::h(*p);
                    if (i+1 < Count) os << ", ";
                }
                if (Count > 1) os << " ]";
                break;
            }
            case 6 /*string*/:
                // Count is explicitly ignored - always 1 for string
                os << '"' << &buf[Offset] << '"';
                break;
            case 7 /*bin*/:
            {
                size_t i=0;
                for (; i<Count && buf[Offset+i] == 0; ++i);
                if (i>4)
                    os << "<" << i << " of \\x00>";
                else
                    i=0;
                for (; i<Count; ++i)
                    outcombi(os, buf[Offset+i]);
                break;
            }
            case 8 /*stringarray*/:
            {
                os << "[\n";
                for (size_t i=0, pos=Offset; i<Count; ++i)
                {
                    std::string tmp(&buf[pos]);
                    os << "\t\t\t\"" << tmp << "\"\n";
                    pos += tmp.length() + 1;
                }
                os << "\t\t]";
                break;
            }
            default:
                os << "UNKNOWN";
        }
    }
};

struct header_t
{
    header_entry_t Entry;
    std::list<index_entry_t> Index;
    std::vector<char> Data;

    template<typename IS>
    void read(IS& is)
    {
        Entry.read(is);
        size_t cnt = Entry.Count;
        for (size_t i=0; i<cnt; ++i)
        {
            index_entry_t ie;
            ie.read(is);
            Index.push_back(ie);
        }
        read_buf(is, Data, Entry.Bytes);
    }
};

struct rpm_t
{
    lead_t Lead;
    header_t Signature;
    std::vector<char> SignaturePadding;
    header_t Header;

    template<typename IS>
    void read(IS& is)
    {
        Lead.read(is);
        Signature.read(is);

        // signature padding (to 8 byte multiples)
        size_t sigpad = is.tellg() % 8;
        read_buf(is, SignaturePadding, sigpad);

        Header.read(is);
    }
};


template<typename CH, typename TR>
std::basic_ostream<CH,TR>& operator<<(std::basic_ostream<CH,TR>& os, const lead_t& v)
{
    os << "LEAD: {\n";
    os << "\tMagic: "; for (size_t i=0; i<lead_t::MagicSize; ++i) outcombi(os, v.Magic[i]); os << '\n';
    os << "\tRpm File Version: " << static_cast<unsigned int>(v.Major) << '.' << static_cast<unsigned int>(v.Minor) << '\n';
    os << "\tPackage Type: " << v.Type << " (" << v.getPackageType() << ")\n";
    os << "\tArchitecture: " << v.ArchNum << "\n";
    os << "\tName: "; {
        size_t i=0;
        for (; i<lead_t::NameSize && v.Name[i] != 0; ++i)
            os << v.Name[i];
        os << "   ...trailing bytes:";
        for (; i<lead_t::NameSize; ++i) outcombi(os, v.Name[i]);
        os << '\n';
    }
    os << "\tOperating System: " << v.OsNum << "\n";
    os << "\tSignature Type: " << v.SignatureType << "\n";
    os << "\tReserved: "; for (size_t i=0; i<lead_t::ReservedSize; ++i) outcombi(os, v.Reserved[i]); os << '\n';
    os << "}\n";
    return os;
}

template<typename CH, typename TR>
std::basic_ostream<CH,TR>& operator<<(std::basic_ostream<CH,TR>& os, const header_entry_t& v)
{
    os << "\tMagic: "; for (size_t i=0; i<header_entry_t::MagicSize; ++i) outcombi(os, v.Magic[i]); os << '\n';
    os << "\tHeader Version: " << static_cast<unsigned int>(v.Version) << '\n';
    os << "\tReserved: "; for (size_t i=0; i<header_entry_t::ReservedSize; ++i) outcombi(os, v.Reserved[i]); os << '\n';
    os << "\tEntry Count: " << v.Count << "\n";
    os << "\tBytes: " << v.Bytes << "\n";
    return os;
}

template<typename T>
struct value_dumper
{
    const T& v_;
    const std::function<std::string(int)> namegetter_;
};

template<typename T>
value_dumper<T> make_value_dumper(const T& v, std::function<std::string(int)> namegetter)
{
    return value_dumper<T>{v, namegetter};
}

template<typename CH, typename TR>
std::basic_ostream<CH,TR>& operator<<(std::basic_ostream<CH,TR>& os, const value_dumper<index_entry_t>& v)
{
    os << "Tag: " << v.v_.Tag << " (" << v.namegetter_(v.v_.Tag) << "), Type: " << v.v_.Type << " (" << TagTypeName(v.v_.Type) << "), Offset: " << v.v_.Offset << ", Count: " << v.v_.Count;
    return os;
}

template<typename CH, typename TR>
std::basic_ostream<CH,TR>& operator<<(std::basic_ostream<CH,TR>& os, const value_dumper<header_t>& v)
{
    os << "{\n" << v.v_.Entry;
    os << "\tINDEX: [\n";
    size_t i = 0;
    for (const auto& ie : v.v_.Index)
    {
        os << "\t\t" << i << ": { " << make_value_dumper(ie, v.namegetter_) << " }\n";
        ++i;
    }
    os << "\t]\n";
    i = 0;
    os << "\tDATA: [\n";
    for (const auto& ie : v.v_.Index)
    {
        os << "\t\t" << i << ": ";
        ie.dump(os, v.v_.Data);
        os << "\n";
        ++i;
    }
    os << "\t]\n";
    os << "}\n";
    return os;
}

template<typename CH, typename TR>
std::basic_ostream<CH,TR>& operator<<(std::basic_ostream<CH,TR>& os, const rpm_t& rpm)
{
    os << rpm.Lead;
    os << "SIGNATURE: " << make_value_dumper(rpm.Signature, SigTagName);
    os << "PADDING: {\n\t\t";
    for (char ch : rpm.SignaturePadding)
    {
        outcombi(os, ch);
    }
    os << "\n}\n";

    os << "HEADER: {\n" << make_value_dumper(rpm.Header, TagName);
}

int main(int argc, const char** argv)
{
    if (argc<2 || argv == nullptr || argv[0]== nullptr)
    {
        std::cerr << "No input file\n";
        return 1;
    }

    std::ifstream f(argv[1], std::ios::binary);
    if (!f.is_open())
    {
        std::cerr << "Cannot open '" << argv[1] << "'.\n";
        return 2;
    }

    rpm_t rpm;
    rpm.read(f);
    std:: cout << rpm;
}
