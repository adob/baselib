#include "quote.h"

#include "lib/io/io.h"
#include "lib/utf8/decode.h"
#include "lib/utf8/encode.h"
#include "lib/utf8/utf8.h"
#include "isprint.h"

using namespace lib;
using namespace lib::strconv;
using namespace lib::strconv::detail;

bool strconv::can_backquote(str s) {
    while (len(s) > 0) {
        int wid;
		rune r = utf8::decode_rune(s, wid);
		s = s+wid;
		if (wid > 1) {
			if (r == u'\ufeff') {
				return false; // BOMs are invisible and should not be quoted.
			}
			continue; // All other multibyte runes are correctly encoded and assumed printable.
		}
		if (r == utf8::RuneError) {
			return false;
		}
		if ((r < ' ' && r != '\t') || r == '`' || r == u'\u007F') {
			return false;
		}
	}
	return true;
}

static bool can_backquote_state(str s, bool eof, utf8::RuneDecoder &state) {

    for (;;) {
        int bytes_consumed;
        bool ok, is_valid;

        rune r = state.decode_rune(s, eof, bytes_consumed, ok, is_valid);
        if (!ok) {
            return true;
        }
        if (!is_valid) {
            return false;
        }

        if (r == u'\ufeff') {
            return false; // BOMs are invisible and should not be quoted.
        }

        if ((r < ' ' && r != '\t') || r == '`' || r == u'\u007F') {
            return false;
        }

        s = s + bytes_consumed;
    }

    return true;
}

bool strconv::can_backquote(io::WriterTo const &w) {
    struct Writer : io::Writer {
        bool ok = true;
        utf8::RuneDecoder state;

        size direct_write(str data, error err) override {
            check(data, false);
            return len(data);
        }

        void check(str data, bool eof) {
            bool ok = can_backquote_state(data, eof, state);
            if (!ok) {
                this->ok = false;
            }
        }
    } writer;

    w.write_to(writer, error::ignore);
    writer.check("", true);
    return writer.ok;
}


// bsearch returns the smallest i such that a[i] >= x.
// If there is no such i, bsearch returns len(a).
template <typename T>
static size bsearch(arr<T> a, uint32 x) {
    size i = 0, j = len(a);
    while (i < j) {
        size h = i + (j-i)/2;
        if (a[h] < x) {
            i = h + 1;
        } else {
            j = h;
        }
    }
    return i;
}


// isInGraphicList reports whether the rune is in the isGraphic list. This separation
// from IsGraphic allows quoteWith to avoid two calls to IsPrint.
// Should be called only if IsPrint fails.
static bool is_in_graphic_list(rune r) {
	// We know r must fit in 16 bits - see makeisprint.go.
	if (r > 0xFFFF) {
		return false;
	}
	bool found = bsearch(IsGraphic, uint16(r)) != len(IsGraphic);
	return found;
}
            
static void quote_with(io::Writer &out, str s, char quote, bool ascii_only, error &err) {
    const bool graphic_only = false;
    out.write_byte(quote, err);
    
    for (int width = 0; len(s) > 0; s = s+width) {
        byte b = s[0];
        rune r = b;
        width = 1; 
        if (r >= utf8::RuneSelf) {
            r = utf8::decode_rune(s, width);
        }
        if (width == 1 && r == utf8::RuneError) {
            out.write("\\x", err);
            out.write_byte(lowerhex[b>>4], err);
            out.write_byte(lowerhex[b&0xF], err);
            continue;
        }
        if (r == (rune) quote || r == '\\') { // always backslashed
            out.write_byte('\\', err);
            out.write_byte(b, err);;
            continue;
        }
        if (ascii_only) {
            if (r < utf8::RuneSelf && is_print(r)) {
                out.write_byte((char) r, err);
                continue;
            }
        } else if (is_print(r) || (graphic_only && is_in_graphic_list(r))) {
            out.write(s[0,width], err);
            continue;
        }
        switch (r) {
            case '\a': out.write("\\a", err); break;
            case '\b': out.write("\\b", err); break;
            case '\f': out.write("\\f", err); break;
            case '\n': out.write("\\n", err); break;
            case '\r': out.write("\\r", err); break;
            case '\t': out.write("\\t", err); break;
            case '\v': out.write("\\v", err); break;
            default:
                if (r < ' ' || r == 0x7f) {
                    out.write("\\x", err); 
                    out.write_byte(lowerhex[b>>4], err);
                    out.write_byte(lowerhex[b&0xF], err);
                    break;
                } 
                
                if (!utf8::valid_rune(r)) {
                    r = 0xFFFD;
                }
			    
                if (r < 0x10000) {
                    out.write("\\u", err);
                    for (int si = 12; si >= 0; si -= 4) {
                        out.write_byte(lowerhex[ (r>>si) & 0xF ], err);
                    }
                } else {
                    out.write("\\U", err);
                    for (int si = 28; si >= 0; si -= 4) {
                        out.write_byte(lowerhex[ (r>>si) & 0xF ], err);
                    }
                }
        }
    }
    out.write_byte(quote, err);
}

static void quote_with_state(io::Writer &out, utf8::RuneDecoder &state, str s, bool eof, char quote, bool ascii_only, error &err) {
    const bool graphic_only = false;
    // out.write(quote, err);
    
    for (;;) {
        int bytes_consumed = 1;
        bool ok, is_valid = true;
        rune r = state.decode_rune(s, eof, bytes_consumed, ok, is_valid);
        if (!ok) {
            return;
        }
        s = s + bytes_consumed;
        
        if (!is_valid) {
            out.write("\\x", err);
            out.write_byte(lowerhex[r>>4], err);
            out.write_byte(lowerhex[r&0xF], err);
            continue;
        }
        if (r == (rune) quote || r == '\\') { // always backslashed
            out.write_byte('\\', err);
            out.write_byte(byte(r), err);;
            continue;
        }
        if (ascii_only) {
            if (r < utf8::RuneSelf && is_print(r)) {
                out.write_byte((char) r, err);
                continue;
            }
        } else if (is_print(r) || (graphic_only && is_in_graphic_list(r))) {
            Array<byte, utf8::UTFMax> buffer;
            out.write(utf8::encode(buffer, r), err);
            continue;
        }
        switch (r) {
            case '\a': out.write("\\a", err); break;
            case '\b': out.write("\\b", err); break;
            case '\f': out.write("\\f", err); break;
            case '\n': out.write("\\n", err); break;
            case '\r': out.write("\\r", err); break;
            case '\t': out.write("\\t", err); break;
            case '\v': out.write("\\v", err); break;
            default:
                if (r < ' ' || r == 0x7f) {
                    out.write("\\x", err); 
                    out.write_byte(lowerhex[byte(r)>>4], err);
                    out.write_byte(lowerhex[byte(r)&0xF], err);
                    break;
                } 
                
                if (!utf8::valid_rune(r)) {
                    r = 0xFFFD;
                }
			    
                if (r < 0x10000) {
                    out.write("\\u", err);
                    for (int si = 12; si >= 0; si -= 4) {
                        out.write_byte(lowerhex[ (r>>si) & 0xF ], err);
                    }
                } else {
                    out.write("\\U", err);
                    for (int si = 28; si >= 0; si -= 4) {
                        out.write_byte(lowerhex[ (r>>si) & 0xF ], err);
                    }
                }
        }
    }
    // out.write(quote, err);
}



void Quoter::write_to(io::Writer &out, error err) const {
    quote_with(out, s, '"', ascii_only, err);
}

Quoter strconv::quote(str s, bool ascii_only) {
    return Quoter(s, ascii_only);
}


void RuneQuoter::write_to(io::Writer &out, error err) const {
    Array<char, utf8::UTFMax> buf;
    quote_with(out, utf8::encode(buf, r), '\'', ascii_only, err);
}

RuneQuoter strconv::quote(rune r, bool ascii_only) {
    return RuneQuoter(r, ascii_only);
}

Quoter strconv::quote_to_ascii(str s) {
    return quote(s, true);
}



bool strconv::is_print(rune r) {
    // Fast check for Latin-1
    if (r <= 0xFF) {
        if (0x20 <= r && r <= 0x7E) {
            // All the ASCII is printable from space through DEL-1.
            return true;
        }
        if (0xA1 <= r && r <= 0xFF) {
            // Similarly for ¡ through ÿ...
            return r != 0xAD; // ...except for the bizarre soft hyphen.
        }
        return false;
    }
    
    // Same algorithm, either on uint16 or uint32 value.
    // First, find first i such that isPrint[i] >= x.
    // This is the index of either the start or end of a pair that might span x.
    // The start is even (isPrint[i&^1]) and the end is odd (isPrint[i|1]).
    // If we find x in a range, make sure x is not in isNotPrint list.
    
    if (r < (1<<16)) {
        size i = bsearch(IsPrint16, r);
        if (i >= len(IsPrint16) || r < IsPrint16[i & ~1] || IsPrint16[i|1] < r) {
            return false;
        }
        size j = bsearch(IsNotPrint16, r);
        return j >= len(IsNotPrint16) || IsNotPrint16[j] != r;
    }
    
    size i = bsearch(IsPrint32, r);
    if (i >= len(IsPrint32) || r < IsPrint32[i & ~1] || IsPrint32[i|1] < r) {
        return false;
    }
    if (r >= 0x20000) {
        return true;
    }
    r -= 0x10000;
    size j = bsearch(IsNotPrint32, r);
    return j >= len(IsNotPrint32) || IsNotPrint32[j] != r;
}

RuneQuoter strconv::quote_rune(rune r) {
    return quote(r, false);
}

RuneQuoter strconv::quote_rune_to_ascii(rune r) {
    return quote(r, true);
}

WriterToQuoter strconv::quote(io::WriterTo const &w, bool ascii_only) {
    WriterToQuoter wtq;
    wtq.w = &w;
    wtq.ascii_only = ascii_only;
    return wtq;
}


void WriterToQuoter::write_to(io::Writer &out, error err) const {
    struct Writer : io::Writer {
        io::Writer *out;
        bool ascii_only;
        utf8::RuneDecoder state;

        size direct_write(str data, error err) override {
            quote_with_state(*this->out, this->state, data, false, '"', this->ascii_only, err);

            return len(data);
        }
    } writer;

    writer.out = &out;
    writer.ascii_only = this->ascii_only;

    out.write_byte('"', err);
    this->w->write_to(writer, err);
    quote_with_state(out, writer.state, "", true, '"', ascii_only, err);
    out.write_byte('"', err);
}

WriterToQuoter strconv::quote_to_ascii(io::WriterTo const &w) {
    return quote(w, true);
}

