# BACnet/IP Header Files and Encoding Functions

## Search Summary
Comprehensive search of the BACnet stack component at `c:\esp\BACnet-ESP32-S3\components\bacnet-stack\src\bacnet\datalink\` for BACnet/IP BVLC (BACnet Virtual Link Control) related header files and encoding functions.

---

## Header Files Located

### 1. **bvlc.h** - BACnet/IP (IPv4) BVLL Control
**Path:** [c:\esp\BACnet-ESP32-S3\components\bacnet-stack\src\bacnet\datalink\bvlc.h](c:\esp\BACnet-ESP32-S3\components\bacnet-stack\src\bacnet\datalink\bvlc.h)

**Description:** BACnet/IP virtual link control module encode and decode for IPv4

**Key Constants:**
- `BVLL_TYPE_BACNET_IP` (0x81)
- `BIP_ADDRESS_MAX` (6 bytes)
- Message types: BVLC_RESULT, BVLC_ORIGINAL_UNICAST_NPDU, BVLC_ORIGINAL_BROADCAST_NPDU, etc.

---

### 2. **bvlc6.h** - BACnet/IPv6 BVLL Control
**Path:** [c:\esp\BACnet-ESP32-S3\components\bacnet-stack\src\bacnet\datalink\bvlc6.h](c:\esp\BACnet-ESP32-S3\components\bacnet-stack\src\bacnet\datalink\bvlc6.h)

**Description:** Implementation of BACnet Virtual Link Layer using IPv6

**Key Constants:**
- `BVLL_TYPE_BACNET_IP6` (0x82)
- `BIP6_ADDRESS_MAX` (18 bytes)
- `BIP6_MULTICAST_GROUP_ID` (0xBAC0)
- Message types: BVLC6_RESULT, BVLC6_ORIGINAL_UNICAST_NPDU, BVLC6_ORIGINAL_BROADCAST_NPDU, etc.

---

### 3. **bip.h** - BACnet/IP DataLink API
**Path:** [c:\esp\BACnet-ESP32-S3\components\bacnet-stack\src\bacnet\datalink\bip.h](c:\esp\BACnet-ESP32-S3\components\bacnet-stack\src\bacnet\datalink\bip.h)

**Description:** BACnet/IP datalink API for IPv4

**Key Constants:**
- `BIP_HEADER_MAX` (4 bytes)
- `BIP_MPDU_MAX` (BIP_HEADER_MAX + MAX_PDU)

---

### 4. **bip6.h** - BACnet/IPv6 DataLink API
**Path:** [c:\esp\BACnet-ESP32-S3\components\bacnet-stack\src\bacnet\datalink\bip6.h](c:\esp\BACnet-ESP32-S3\components\bacnet-stack\src\bacnet\datalink\bip6.h)

**Description:** BACnet/IPv6 datalink API

**Key Constants:**
- `BIP6_HEADER_MAX` (4 bytes)
- `BIP6_MPDU_MAX` (BIP6_HEADER_MAX + MAX_PDU)

---

## BACnet/IP (IPv4) BVLC Encoding Functions

### Header Encoding/Decoding
- `int bvlc_encode_header(uint8_t *pdu, uint16_t pdu_size, uint8_t message_type, uint16_t length)`
- `int bvlc_decode_header(const uint8_t *pdu, uint16_t pdu_len, uint8_t *message_type, uint16_t *length)`

### Address Encoding/Decoding
- `int bvlc_encode_address(uint8_t *pdu, uint16_t pdu_size, const BACNET_IP_ADDRESS *ip_address)`
- `int bvlc_decode_address(const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_ADDRESS *ip_address)`
- `bool bvlc_address_copy(BACNET_IP_ADDRESS *dst, const BACNET_IP_ADDRESS *src)`
- `bool bvlc_address_different(const BACNET_IP_ADDRESS *dst, const BACNET_IP_ADDRESS *src)`
- `bool bvlc_address_from_ascii(BACNET_IP_ADDRESS *dst, const char *addrstr)`
- `bool bvlc_address_port_from_ascii(BACNET_IP_ADDRESS *dst, const char *addrstr, const char *portstr)`
- `void bvlc_address_from_network(BACNET_IP_ADDRESS *dst, uint32_t addr)`
- `bool bvlc_address_set(BACNET_IP_ADDRESS *addr, uint8_t addr0, uint8_t addr1, uint8_t addr2, uint8_t addr3)`
- `bool bvlc_address_get(const BACNET_IP_ADDRESS *addr, uint8_t *addr0, uint8_t *addr1, uint8_t *addr2, uint8_t *addr3)`

### Address Conversion
- `bool bvlc_ip_address_to_bacnet_local(BACNET_ADDRESS *addr, const BACNET_IP_ADDRESS *ipaddr)`
- `bool bvlc_ip_address_from_bacnet_local(BACNET_IP_ADDRESS *ipaddr, const BACNET_ADDRESS *addr)`
- `bool bvlc_ip_address_to_bacnet_remote(BACNET_ADDRESS *addr, uint16_t dnet, const BACNET_IP_ADDRESS *ipaddr)`
- `bool bvlc_ip_address_from_bacnet_remote(BACNET_IP_ADDRESS *ipaddr, uint16_t *dnet, const BACNET_ADDRESS *addr)`

### Broadcast Distribution Mask Functions
- `int bvlc_encode_broadcast_distribution_mask(uint8_t *pdu, uint16_t pdu_size, const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *bd_mask)`
- `int bvlc_decode_broadcast_distribution_mask(const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_BROADCAST_DISTRIBUTION_MASK *bd_mask)`
- `void bvlc_broadcast_distribution_mask_set(BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask, uint8_t addr0, uint8_t addr1, uint8_t addr2, uint8_t addr3)`
- `void bvlc_broadcast_distribution_mask_get(const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask, uint8_t *addr0, uint8_t *addr1, uint8_t *addr2, uint8_t *addr3)`
- `bool bvlc_broadcast_distribution_mask_different(const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *dst, const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *src)`
- `bool bvlc_broadcast_distribution_mask_copy(BACNET_IP_BROADCAST_DISTRIBUTION_MASK *dst, const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *src)`
- `bool bvlc_broadcast_distribution_mask_from_host(BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask, uint32_t broadcast_mask)`
- `bool bvlc_broadcast_distribution_mask_to_host(uint32_t *broadcast_mask, const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask)`
- `bool bvlc_address_mask(BACNET_IP_ADDRESS *dst, const BACNET_IP_ADDRESS *src, const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask)`

### Broadcast Distribution Table (BDT) Functions
- `int bvlc_encode_broadcast_distribution_table_entry(uint8_t *pdu, uint16_t pdu_size, const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry)`
- `int bvlc_decode_broadcast_distribution_table_entry(const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry)`
- `int bvlc_encode_write_broadcast_distribution_table(uint8_t *pdu, uint16_t pdu_size, BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)`
- `int bvlc_decode_write_broadcast_distribution_table(const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)`
- `int bvlc_encode_read_broadcast_distribution_table(uint8_t *pdu, uint16_t pdu_size)`
- `int bvlc_encode_read_broadcast_distribution_table_ack(uint8_t *pdu, uint16_t pdu_size, BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)`
- `int bvlc_decode_read_broadcast_distribution_table_ack(const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)`
- `int bvlc_broadcast_distribution_table_entry_encode(uint8_t *apdu, const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry)`
- `int bvlc_broadcast_distribution_table_list_encode(uint8_t *apdu, const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head)`
- `int bvlc_broadcast_distribution_table_encode(uint8_t *apdu, uint16_t apdu_size, const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head)`
- `int bvlc_broadcast_distribution_table_decode(const uint8_t *apdu, uint16_t apdu_len, BACNET_ERROR_CODE *error_code, BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head)`
- `void bvlc_broadcast_distribution_table_link_array(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list, const size_t bdt_array_size)`
- `uint16_t bvlc_broadcast_distribution_table_count(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)`
- `uint16_t bvlc_broadcast_distribution_table_valid_count(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)`
- `void bvlc_broadcast_distribution_table_valid_clear(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)`
- `bool bvlc_broadcast_distribution_table_entry_different(const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *dst, const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *src)`
- `bool bvlc_broadcast_distribution_table_entry_copy(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *dst, const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *src)`
- `bool bvlc_broadcast_distribution_table_entry_append(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list, const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry)`
- `bool bvlc_broadcast_distribution_table_entry_insert(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list, const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry, uint16_t array_index)`
- `bool bvlc_broadcast_distribution_table_entry_set(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry, const BACNET_IP_ADDRESS *addr, const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask)`
- `bool bvlc_broadcast_distribution_table_entry_forward_address(BACNET_IP_ADDRESS *addr, const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry)`

### Foreign Device Table (FDT) Functions
- `int bvlc_encode_foreign_device_table_entry(uint8_t *pdu, uint16_t pdu_size, const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry)`
- `int bvlc_decode_foreign_device_table_entry(const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry)`
- `int bvlc_encode_read_foreign_device_table(uint8_t *pdu, uint16_t pdu_size)`
- `int bvlc_encode_read_foreign_device_table_ack(uint8_t *pdu, uint16_t pdu_size, BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list)`
- `int bvlc_decode_read_foreign_device_table_ack(const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list)`
- `int bvlc_foreign_device_table_entry_encode(uint8_t *apdu, const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head)`
- `int bvlc_foreign_device_table_list_encode(uint8_t *apdu, const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head)`
- `int bvlc_foreign_device_table_encode(uint8_t *apdu, uint16_t apdu_size, const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head)`
- `int bvlc_foreign_device_table_decode(const uint8_t *apdu, uint16_t apdu_len, BACNET_ERROR_CODE *error_code, BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head)`
- `void bvlc_foreign_device_table_maintenance_timer(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list, uint16_t seconds)`
- `void bvlc_foreign_device_table_valid_clear(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list)`
- `uint16_t bvlc_foreign_device_table_valid_count(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list)`
- `uint16_t bvlc_foreign_device_table_count(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list)`
- `void bvlc_foreign_device_table_link_array(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list, const size_t array_size)`
- `bool bvlc_foreign_device_table_entry_different(const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *dst, const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *src)`
- `bool bvlc_foreign_device_table_entry_copy(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *dst, const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *src)`
- `bool bvlc_foreign_device_table_entry_delete(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list, const BACNET_IP_ADDRESS *ip_address)`
- `bool bvlc_foreign_device_table_entry_add(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list, const BACNET_IP_ADDRESS *ip_address, uint16_t ttl_seconds)`
- `bool bvlc_foreign_device_table_entry_insert(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list, const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry, uint16_t array_index)`

### BVLC Message Encoding/Decoding Functions

#### Result Message
- `int bvlc_encode_result(uint8_t *pdu, uint16_t pdu_size, uint16_t result_code)`
- `int bvlc_decode_result(const uint8_t *pdu, uint16_t pdu_len, uint16_t *result_code)`
- `const char *bvlc_result_code_name(uint16_t result_code)`

#### Original Unicast NPDU
- `int bvlc_encode_original_unicast(uint8_t *pdu, uint16_t pdu_size, const uint8_t *npdu, uint16_t npdu_len)`
- `int bvlc_decode_original_unicast(const uint8_t *pdu, uint16_t pdu_len, uint8_t *npdu, uint16_t npdu_size, uint16_t *npdu_len)`

#### Original Broadcast NPDU
- `int bvlc_encode_original_broadcast(uint8_t *pdu, uint16_t pdu_size, const uint8_t *npdu, uint16_t npdu_len)`
- `int bvlc_decode_original_broadcast(const uint8_t *pdu, uint16_t pdu_len, uint8_t *npdu, uint16_t npdu_size, uint16_t *npdu_len)`

#### Forwarded NPDU
- `int bvlc_encode_forwarded_npdu(uint8_t *pdu, uint16_t pdu_size, const BACNET_IP_ADDRESS *address, const uint8_t *npdu, uint16_t npdu_len)`
- `int bvlc_decode_forwarded_npdu(const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_ADDRESS *address, uint8_t *npdu, uint16_t npdu_size, uint16_t *npdu_len)`

#### Foreign Device Registration
- `int bvlc_encode_register_foreign_device(uint8_t *pdu, uint16_t pdu_size, uint16_t ttl_seconds)`
- `int bvlc_decode_register_foreign_device(const uint8_t *pdu, uint16_t pdu_len, uint16_t *ttl_seconds)`

#### Delete Foreign Device
- `int bvlc_encode_delete_foreign_device(uint8_t *pdu, uint16_t pdu_size, const BACNET_IP_ADDRESS *ip_address)`
- `int bvlc_decode_delete_foreign_device(const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_ADDRESS *ip_address)`

#### Secure BVLL
- `int bvlc_encode_secure_bvll(uint8_t *pdu, uint16_t pdu_size, const uint8_t *sbuf, uint16_t sbuf_len)`
- `int bvlc_decode_secure_bvll(const uint8_t *pdu, uint16_t pdu_len, uint8_t *sbuf, uint16_t sbuf_size, uint16_t *sbuf_len)`

#### Distribute Broadcast to Network
- `int bvlc_encode_distribute_broadcast_to_network(uint8_t *pdu, uint16_t pdu_size, const uint8_t *npdu, uint16_t npdu_len)`
- `int bvlc_decode_distribute_broadcast_to_network(const uint8_t *pdu, uint16_t pdu_len, uint8_t *npdu, uint16_t npdu_size, uint16_t *npdu_len)`

#### BBMD Address Encoding
- `int bvlc_foreign_device_bbmd_host_address_encode(uint8_t *apdu, uint16_t apdu_size, const BACNET_IP_ADDRESS *ip_address)`
- `int bvlc_foreign_device_bbmd_host_address_decode(const uint8_t *apdu, uint16_t apdu_len, BACNET_ERROR_CODE *error_code, BACNET_IP_ADDRESS *ip_address)`

---

## BACnet/IPv6 BVLC Encoding Functions

### Header Encoding/Decoding
- `int bvlc6_encode_header(uint8_t *pdu, uint16_t pdu_size, uint8_t message_type, uint16_t length)`
- `int bvlc6_decode_header(const uint8_t *pdu, uint16_t pdu_len, uint8_t *message_type, uint16_t *length)`

### Address Encoding/Decoding
- `int bvlc6_encode_address(uint8_t *pdu, uint16_t pdu_size, const BACNET_IP6_ADDRESS *ip6_address)`
- `int bvlc6_decode_address(const uint8_t *pdu, uint16_t pdu_len, BACNET_IP6_ADDRESS *ip6_address)`
- `bool bvlc6_address_copy(BACNET_IP6_ADDRESS *dst, const BACNET_IP6_ADDRESS *src)`
- `bool bvlc6_address_different(const BACNET_IP6_ADDRESS *dst, const BACNET_IP6_ADDRESS *src)`
- `int bvlc6_address_to_ascii(const BACNET_IP6_ADDRESS *addr, char *buf, size_t buf_size)`
- `bool bvlc6_address_from_ascii(BACNET_IP6_ADDRESS *addr, const char *addrstr)`
- `bool bvlc6_address_n_port_set(BACNET_IP6_ADDRESS *addr, uint8_t *addr16, uint16_t port)`
- `bool bvlc6_address_set(BACNET_IP6_ADDRESS *addr, uint16_t addr0, uint16_t addr1, uint16_t addr2, uint16_t addr3, uint16_t addr4, uint16_t addr5, uint16_t addr6, uint16_t addr7)`
- `bool bvlc6_address_get(const BACNET_IP6_ADDRESS *addr, uint16_t *addr0, uint16_t *addr1, uint16_t *addr2, uint16_t *addr3, uint16_t *addr4, uint16_t *addr5, uint16_t *addr6, uint16_t *addr7)`

### Virtual MAC Address (VMAC) Functions
- `bool bvlc6_vmac_address_set(BACNET_ADDRESS *addr, uint32_t device_id)`
- `bool bvlc6_vmac_address_get(const BACNET_ADDRESS *addr, uint32_t *device_id)`

### BVLC Message Functions

#### Result Message
- `int bvlc6_encode_result(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac, uint16_t result_code)`
- `int bvlc6_decode_result(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac, uint16_t *result_code)`

#### Original Unicast NPDU
- `int bvlc6_encode_original_unicast(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, uint32_t vmac_dst, const uint8_t *npdu, uint16_t npdu_len)`
- `int bvlc6_decode_original_unicast(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac_src, uint32_t *vmac_dst, uint8_t *npdu, uint16_t npdu_size, uint16_t *npdu_len)`

#### Original Broadcast NPDU
- `int bvlc6_encode_original_broadcast(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac, const uint8_t *npdu, uint16_t npdu_len)`
- `int bvlc6_decode_original_broadcast(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac, uint8_t *npdu, uint16_t npdu_size, uint16_t *npdu_len)`

#### Address Resolution
- `int bvlc6_encode_address_resolution(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, uint32_t vmac_target)`
- `int bvlc6_decode_address_resolution(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac_src, uint32_t *vmac_target)`

#### Forwarded Address Resolution
- `int bvlc6_encode_forwarded_address_resolution(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, uint32_t vmac_target, const BACNET_IP6_ADDRESS *bip6_address)`
- `int bvlc6_decode_forwarded_address_resolution(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac_src, uint32_t *vmac_target, BACNET_IP6_ADDRESS *bip6_address)`

#### Address Resolution ACK
- `int bvlc6_encode_address_resolution_ack(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, uint32_t vmac_dst)`
- `int bvlc6_decode_address_resolution_ack(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac_src, uint32_t *vmac_dst)`

#### Virtual Address Resolution
- `int bvlc6_encode_virtual_address_resolution(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src)`
- `int bvlc6_decode_virtual_address_resolution(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac_src)`

#### Virtual Address Resolution ACK
- `int bvlc6_encode_virtual_address_resolution_ack(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, uint32_t vmac_dst)`
- `int bvlc6_decode_virtual_address_resolution_ack(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac_src, uint32_t *vmac_dst)`

#### Forwarded NPDU
- `int bvlc6_encode_forwarded_npdu(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, const BACNET_IP6_ADDRESS *address, const uint8_t *npdu, uint16_t npdu_len)`
- `int bvlc6_decode_forwarded_npdu(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac_src, BACNET_IP6_ADDRESS *address, uint8_t *npdu, uint16_t npdu_size, uint16_t *npdu_len)`

#### Foreign Device Registration
- `int bvlc6_encode_register_foreign_device(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, uint16_t ttl_seconds)`
- `int bvlc6_decode_register_foreign_device(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac_src, uint16_t *ttl_seconds)`

#### Delete Foreign Device
- `int bvlc6_encode_delete_foreign_device(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, const BACNET_IP6_ADDRESS *bip6_address)`
- `int bvlc6_decode_delete_foreign_device(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac_src, BACNET_IP6_ADDRESS *bip6_address)`

#### Secure BVLL
- `int bvlc6_encode_secure_bvll(uint8_t *pdu, uint16_t pdu_size, const uint8_t *sbuf, uint16_t sbuf_len)`
- `int bvlc6_decode_secure_bvll(const uint8_t *pdu, uint16_t pdu_len, uint8_t *sbuf, uint16_t sbuf_size, uint16_t *sbuf_len)`

#### Distribute Broadcast to Network
- `int bvlc6_encode_distribute_broadcast_to_network(uint8_t *pdu, uint16_t pdu_size, uint32_t vmac, const uint8_t *npdu, uint16_t npdu_len)`
- `int bvlc6_decode_distribute_broadcast_to_network(const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac, uint8_t *npdu, uint16_t npdu_size, uint16_t *npdu_len)`

### Broadcast Distribution Table Functions
- `int bvlc6_foreign_device_bbmd_host_address_encode(uint8_t *apdu, uint16_t apdu_size, const BACNET_IP6_ADDRESS *ip6_address)`
- `int bvlc6_broadcast_distribution_table_entry_encode(uint8_t *apdu, const BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry)`
- `int bvlc6_broadcast_distribution_table_list_encode(uint8_t *apdu, BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head)`
- `int bvlc6_broadcast_distribution_table_encode(uint8_t *apdu, uint16_t apdu_size, BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head)`

### Foreign Device Table Functions
- `int bvlc6_foreign_device_table_entry_encode(uint8_t *apdu, const BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry)`
- `int bvlc6_foreign_device_table_list_encode(uint8_t *apdu, BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head)`
- `int bvlc6_foreign_device_table_encode(uint8_t *apdu, uint16_t apdu_size, BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head)`

---

## Key Data Structures

### IPv4 Address Structure (bvlc.h)
```c
typedef struct BACnet_IP_Address {
    uint8_t address[IP_ADDRESS_MAX];  // 4 bytes
    uint16_t port;                     // UDP port (big-endian)
} BACNET_IP_ADDRESS;
```

### IPv6 Address Structure (bvlc6.h)
```c
typedef struct BACnet_IP6_Address {
    uint8_t address[IP6_ADDRESS_MAX];  // 16 bytes
    uint16_t port;                      // UDP port (big-endian)
} BACNET_IP6_ADDRESS;
```

### Broadcast Distribution Mask (IPv4)
```c
typedef struct BACnet_IP_Broadcast_Distribution_Mask {
    uint8_t address[IP_ADDRESS_MAX];  // 4 bytes
} BACNET_IP_BROADCAST_DISTRIBUTION_MASK;
```

### Broadcast Distribution Table Entry (IPv4)
```c
typedef struct BACnet_IP_Broadcast_Distribution_Table_Entry {
    bool valid;
    BACNET_IP_ADDRESS dest_address;
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK broadcast_mask;
    struct BACnet_IP_Broadcast_Distribution_Table_Entry *next;
} BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY;
```

### Foreign Device Table Entry (IPv4)
```c
typedef struct BACnet_IP_Foreign_Device_Table_Entry {
    bool valid;
    BACNET_IP_ADDRESS dest_address;
    uint16_t ttl_seconds;
    uint16_t ttl_seconds_remaining;
    struct BACnet_IP_Foreign_Device_Table_Entry *next;
} BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY;
```

---

## Additional Header Files in Datalink Component

Located in `c:\esp\BACnet-ESP32-S3\components\bacnet-stack\src\bacnet\datalink\`:

- arcnet.h - ARCnet protocol support
- automac.c/h - Automatic MAC address handling
- bacsec.c/h - BACnet secure messaging
- bsc/ - Sub-directory for additional secure components
- bzll.h - BACnet Zero Configuration Link Layer
- cobs.c/h - Consistent Overhead Byte Stuffing
- crc.c/h - CRC calculation
- datalink.c/h - Generic datalink layer
- dlenv.c/h - Datalink environment
- dlmstp.c/h - Master-Slave/Token-Passing protocol
- ethernet.h - Ethernet support
- mstp.c/h - MS/TP protocol implementation
- mstpdef.h - MS/TP definitions
- mstptext.c/h - MS/TP text utilities

---

## Summary

Found **2 primary BVLC header files** containing BACnet/IP frame encoding functions:

1. **bvlc.h** - IPv4 encoding (50+ functions)
2. **bvlc6.h** - IPv6 encoding (40+ functions)

**Total encoding/decoding function count: 90+ specialized functions** for:
- BVLC header encoding/decoding
- IP address encoding/decoding
- Unicast/Broadcast NPDU frames
- Foreign Device Table management
- Broadcast Distribution Table management
- Address resolution
- Secure BVLL messaging
