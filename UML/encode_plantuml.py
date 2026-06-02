import sys
import zlib

puml_path = 'UML/plantuml_classes.puml'
try:
    with open(puml_path, 'rb') as f:
        data = f.read()
except Exception as e:
    print('ERROR: cannot read', puml_path, e, file=sys.stderr)
    sys.exit(2)

# raw deflate (no zlib header)
compressobj = zlib.compressobj(level=9, method=zlib.DEFLATED, wbits=-15)
compressed = compressobj.compress(data) + compressobj.flush()

encode64_alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_"

def encode6(data_bytes):
    res_chars = []
    i = 0
    L = len(data_bytes)
    while i < L:
        b1 = data_bytes[i]
        i += 1
        if i >= L:
            res_chars.append(encode_char(b1 >> 2))
            res_chars.append(encode_char((b1 & 0x3) << 4))
            break
        b2 = data_bytes[i]
        i += 1
        if i >= L:
            res_chars.append(encode_char(b1 >> 2))
            res_chars.append(encode_char(((b1 & 0x3) << 4) | (b2 >> 4)))
            res_chars.append(encode_char((b2 & 0xF) << 2))
            break
        b3 = data_bytes[i]
        i += 1
        res_chars.append(encode_char(b1 >> 2))
        res_chars.append(encode_char(((b1 & 0x3) << 4) | (b2 >> 4)))
        res_chars.append(encode_char(((b2 & 0xF) << 2) | (b3 >> 6)))
        res_chars.append(encode_char(b3 & 0x3F))
    return ''.join(res_chars)


def encode_char(v):
    return encode64_alphabet[v & 0x3F]

encoded = encode6(compressed)
print('PNG URL: http://www.plantuml.com/plantuml/png/' + encoded)
print('SVG URL: http://www.plantuml.com/plantuml/svg/' + encoded)
