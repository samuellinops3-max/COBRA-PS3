import struct

def make_sfo():
    title = "Slither.io Offline"
    title_id = "SLIT00001"
    # Monta um PARAM.SFO simples e valido
    keys = ["TITLE", "TITLE_ID", "CATEGORY", "VERSION"]
    vals = [title, title_id, "HG", "01.00"]
    header_len = 20
    key_table_off = header_len + len(keys)*16
    data_table_off = key_table_off + sum(len(k)+1 for k in keys)
    data_off = 0
    entries = b""
    key_table = b""
    data_table = b""
    for k, v in zip(keys, vals):
        v_b = v.encode() + b"\x00"
        entries += struct.pack("<HHIII", len(key_table), 0x0402, len(v_b), len(v_b), data_off)
        key_table += k.encode() + b"\x00"
        data_table += v_b
        data_off += len(v_b)
    header = struct.pack("<IIIIII", 0x46535000, 0x101, key_table_off, data_table_off, len(keys))
    open("PARAM.SFO", "wb").write(header + entries + key_table + data_table)
    print("PARAM.SFO criado!")

make_sfo()