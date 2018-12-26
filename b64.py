# Simple Base64 codec
alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'

def enc1(s, fmt=0):
    """Base64 encode according to MIME RFC 2045 (fmt=1) or PEM RFC 1421
    (fmt=2), optionally following base64url in RFC 4648 (fmt&4)"""
    # 3 octets become 4 sextets (inflates input ~33%): 6 | 2 4 | 4 2 | 6
    # i.o.w. picks 3 bytes and emits 4 (each sextet indexing in the 64 chars alphabet)
    ob = 0 # bytes emitted (not counting CRLF)
    p = 0
    outsize = (len(s)/3+1)*4 # inflates input of ~33%
    if fmt == 1: # this roughly increases size by 4-5%
        outsize += ((outsize/76)+1)*2
    if fmt == 2:
        outsize += ((outsize/64)+1)*2
    result = bytearray(outsize)
    global alphabet
    if fmt & 4:
        alphabet = alphabet.replace('+/', '-_')
    word=0
    todo=0
    for c in s:
        word = (word<<8) | ord(c)
        word &= 0xFFFF # Python has unlimited length integers!
        todo+=8
        while todo >= 6:
            todo-=6
            result[p] = alphabet[(word>>todo) & 0x3F]
            p+=1
            ob+=1
            if (fmt == 1 and not ob%76) or (fmt == 2 and not ob%64):
                result[p:p+2] = '\r\n'
                p+=2
    if todo == 2:
        result[p] = alphabet[(word&3)<<4]
        result[p+1] = '='
        result[p+2] = '='
        p+=3
    elif todo == 4:
        result[p] = alphabet[(word&0xF)<<2]
        result[p+1] = '='
        p+=2
    return str(result[:p])

    
def enc(s, fmt=0):
    """Base64 encode according to MIME RFC 2045 (fmt=1) or PEM RFC 1421
    (fmt=2), optionally following base64url in RFC 4648 (fmt&4)"""
    # 3 octets become 4 sextets (inflates input ~33%): 6 | 2 4 | 4 2 | 6
    # i.o.w. picks 3 bytes and emits 4 (each sextet indexing in the 64 chars alphabet)
    cb = 0 # bytes read from input
    ob = 0 # bytes emitted (not counting CRLF)
    p = 0
    outsize = (len(s)/3+1)*4
    if fmt == 1:
        outsize += ((outsize/76)+1)*2
    if fmt == 2:
        outsize += ((outsize/64)+1)*2
    result = bytearray(outsize)
    global alphabet
    if fmt & 4:
        alphabet = alphabet.replace('+/', '-_')
    for c in s:
        i = cb%3
        if i == 0: # pick 6 bits, push 2
            # 0xFC = 0b11111100
            result[p] = alphabet[(ord(c) & 0xFC) >> 2]
            p+=1
            result[p] = ord(c) & 3 
        elif i == 1: # pop 2, pick 4, push 4
            # 0xF0 = 0b11110000
            result[p] = alphabet[result[p] << 4 | ((ord(c) & 0xF0) >> 4)]
            p+=1
            result[p] = ord(c) & 0xF
        elif i == 2: # pop 4, pick 2, emit last 6
            # 0xC0 = 0b11000000
            result[p] = alphabet[result[p] << 2 | ((ord(c) & 0xC0) >> 6)]
            ob+=1
            p+=1
            # 0x3F = 0b00111111
            result[p] = alphabet[ord(c) & 0x3F]
            p+=1
        cb+=1
        ob+=1
        if (fmt == 1 and not ob%76) or (fmt == 2 and not ob%64):
            result[p:p+2] = '\r\n' # this roughly increases size by 4-5%
            p+=2
    i = 3-i
    if i:
        if i == 3:
            result[p:p+2] = alphabet[result[p] << 4] + '=='
            p+=3
        elif i == 2:
            result[p:p+1] = alphabet[result[p] << 2] + '='
            p+=2
    return str(result[:p])

def enc0(s, fmt=0):
    """Base64 encode according to MIME RFC 2045 (fmt=1) or PEM RFC 1421
    (fmt=2), optionally following base64url in RFC 4648 (fmt&4)"""
    # 3 octets become 4 sextets (inflates input ~33%): 6 | 2 4 | 4 2 | 6
    # i.o.w. picks 3 bytes and emits 4 (each sextet indexing in the 64 chars alphabet)
    cb = 0 # bytes read from input
    ob = 0 # bytes emitted (not counting CRLF)
    result = ''
    global alphabet
    if fmt & 4:
        alphabet = alphabet.replace('+/', '-_')
    for c in s:
        i = cb%3
        if i == 0: # pick 6 bits, push 2
            # 0xFC = 0b11111100
            result += alphabet[(ord(c) & 0xFC) >> 2]
            j = ord(c) & 3
        elif i == 1: # pop 2, pick 4, push 4
            # 0xF0 = 0b11110000
            result += alphabet[j << 4 | ((ord(c) & 0xF0) >> 4)]
            j = ord(c) & 0xF
        elif i == 2: # pop 4, pick 2, emit last 6
            # 0xC0 = 0b11000000
            result += alphabet[j << 2 | ((ord(c) & 0xC0) >> 6)]
            # 0x3F = 0b00111111
            j = ord(c) & 0x3F
            result += alphabet[j]
            j=0
            ob+=1
        cb+=1
        ob+=1
        if (fmt == 1 and not ob%76) or (fmt == 2 and not ob%64):
            result += '\r\n' # this roughly increases size by 4-5%
    i = 3-i
    if i:
        if i == 3:
            result += alphabet[j << 4] + '=='
        elif i == 2:
            result += alphabet[j << 2] + '='
    return result

def dec(s, fmt=0):
    "Base64 decode"
    i = 0
    result = ''
    global alphabet
    if fmt & 4:
        alphabet = alphabet.replace('+/', '-_')
    for c in s:
        if c in ('=', '\r', '\n'): continue
        n = alphabet.find(c)
        if n < 0:
            raise BaseException("BAD INPUT")
        if i == 0: # push 6 bits
            j = n << 2
        elif i == 1: # pop 6, pick 2, push 4
            j |= ((n & 0b110000) >> 4)
            result += chr(j)
            j = (n & 0b001111) << 4
        elif i == 2: # pop 4, pick 4, push 2
            j |= ((n & 0b111100) >> 2)
            result += chr(j)
            j = (n & 0b11) << 6
        elif i == 3: # pop 2, pick 6
            j |= n
            result += chr(j)
        i = (i+1)%4
    return result
    
    
if __name__ == '__main__':
    import base64, sys, timeit
    from ctypes import *
    b64 = CDLL('b64.dll')
    #~ b64.dec.argtypes = [c_char_p, c_int]
    #~ b64.dec.restype = c_char_p
    #~ b64.dec2.restype = c_char_p
    s = file(sys.argv[1],'rb').read()
    b = enc1(s, 1)
    if b.replace('\r\n','') != base64.b64encode(s):
        print 'ENCODER ERROR'
        file('encoded.txt','wb').write(b)
        file('good_encoded.txt','wb').write(base64.b64encode(s))
    if dec(b) != s:
        print 'DECODER ERROR'
    if len(b) < 4096: print b
    print sys.argv[1], len(s), len(b), len(b)/float(len(s))
    print "Time to enc: ", timeit.timeit('enc(s, 1)', setup='from __main__ import enc, s', number=1)
    print "Time to enc1: ", timeit.timeit('enc1(s, 1)', setup='from __main__ import enc1, s', number=1)
    print "Time to enc0: ", timeit.timeit('enc0(s, 1)', setup='from __main__ import enc0, s', number=1)
    #~ open('a.txt','wb').write(base64.b64encode(s))
    #~ open('b.txt','wb').write(c_char_p(b64.enc3(s, len(s), 0)).value)
    #~ for i in range(256):
        #~ a=base64.b64encode(chr(i))
        #~ b=c_char_p(b64.enc3(chr(i), 1, 0)).value
        #~ if a!=b:
            #~ print '%s != %s (%d)' % (a,b,i)
    #~ assert base64.b64encode(s) == c_char_p(b64.enc2(s, len(s), 0)).value
    assert base64.b64encode(s) == c_char_p(b64.enc3(s, len(s), 0)).value
    #~ assert base64.b64encode(s) == c_char_p(b64.enc4(s, len(s), 0)).value
    z = c_void_p(0);
    k = b64.dec2(b, len(b), byref(z), 0)
    assert s == string_at(z, k)
    print "Time to b64encode: ", timeit.timeit('base64.b64encode(s)', setup='from __main__ import base64, s', number=30)
    #~ print "Time to b64.enc: ", timeit.timeit('b64.enc(s, len(s), 0)', setup='from __main__ import b64, s', number=1)
    #~ print "Time to b64.enc1: ", timeit.timeit('b64.enc1(s, len(s), 0)', setup='from __main__ import b64, s', number=1)
    #~ print "Time to b64.enc2: ", timeit.timeit('b64.enc2(s, len(s), 0)', setup='from __main__ import b64, s', number=30)
    print "Time to b64.enc3: ", timeit.timeit('b64.enc3(s, len(s), 0)', setup='from __main__ import b64, s', number=30)
    #~ print "Time to b64.enc4: ", timeit.timeit('b64.enc4(s, len(s), 0)', setup='from __main__ import b64, s', number=30)
    print "Time to b64.dec2: ", timeit.timeit('b64.dec2(b, len(b), byref(z), 0)', setup='from __main__ import b64, b, z, byref', number=30)
    print "Time to b64decode: ", timeit.timeit('base64.b64decode(b)', setup='from __main__ import base64, b', number=30)
