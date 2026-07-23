/* Minimal BACnet/IP implementation for ESP32 with modern IDF */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "esp_netif.h"
#include "lwip/ip4_addr.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/bvlc.h"
#include "bacnet/bacdcode.h"
#include <lwip/sockets.h>

/* Store BACnet/IP addresses */
static BACNET_IP_ADDRESS My_BIP_Address = { {0, 0, 0, 0}, 0xBAC0 };
static BACNET_IP_ADDRESS My_Broadcast_Address = { {255, 255, 255, 255}, 0xBAC0 };

/* UDP socket for BACnet/IP */
static int bip_socket = -1;

/**
 * Initialize BACnet/IP
 * Called once WiFi is connected
 */
bool bip_init(char *ifname)
{
    esp_netif_t *esp_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info = { 0 };
    
    if (esp_netif == NULL) {
        printf("BACnet: Could not get network interface\n");
        return false;
    }
    
    /* Get IP address from network interface */
    if (esp_netif_get_ip_info(esp_netif, &ip_info) != ESP_OK) {
        printf("BACnet: Could not get IP info\n");
        return false;
    }
    
    /* Convert IP address to host order for byte extraction */
    uint32_t ip_addr_val = ntohl(ip_info.ip.addr);
    uint32_t netmask_val = ntohl(ip_info.netmask.addr);
    
    /* Set our address (in network byte order) */
    /* Extract octets from the 32-bit address */
    My_BIP_Address.address[0] = (ip_addr_val >> 24) & 0xFF;
    My_BIP_Address.address[1] = (ip_addr_val >> 16) & 0xFF;
    My_BIP_Address.address[2] = (ip_addr_val >> 8) & 0xFF;
    My_BIP_Address.address[3] = (ip_addr_val >> 0) & 0xFF;
    My_BIP_Address.port = 0xBAC0;  /* Default BACnet/IP port */
    
    /* Calculate broadcast address: (ip & netmask) | ~netmask */
    uint32_t broadcast_val = (ip_addr_val & netmask_val) | (~netmask_val);
    My_Broadcast_Address.address[0] = (broadcast_val >> 24) & 0xFF;
    My_Broadcast_Address.address[1] = (broadcast_val >> 16) & 0xFF;
    My_Broadcast_Address.address[2] = (broadcast_val >> 8) & 0xFF;
    My_Broadcast_Address.address[3] = (broadcast_val >> 0) & 0xFF;
    My_Broadcast_Address.port = 0xBAC0;  /* Default BACnet/IP port */
    
    /* Set BACnet stack addresses */
    bip_set_addr(&My_BIP_Address);
    bip_set_broadcast_addr(&My_Broadcast_Address);
    
    printf("BACnet/IP initialized: IP=%d.%d.%d.%d, Broadcast=%d.%d.%d.%d\n",
           My_BIP_Address.address[0], My_BIP_Address.address[1], 
           My_BIP_Address.address[2], My_BIP_Address.address[3],
           My_Broadcast_Address.address[0], My_Broadcast_Address.address[1], 
           My_Broadcast_Address.address[2], My_Broadcast_Address.address[3]);
    
    /* Create UDP socket for BACnet/IP */
    bip_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (bip_socket < 0) {
        printf("BACnet: Failed to create UDP socket\n");
        return false;
    }
    
    /* Set socket to allow broadcasting */
    int broadcast_enable = 1;
    if (setsockopt(bip_socket, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, 
                   sizeof(broadcast_enable)) < 0) {
        printf("BACnet: Failed to enable broadcast on socket\n");
        close(bip_socket);
        bip_socket = -1;
        return false;
    }
    
    /* Bind socket to BACnet port on all interfaces */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0xBAC0);  /* BACnet/IP port 47808 */
    
    if (bind(bip_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("BACnet: Failed to bind UDP socket to port 0xBAC0\n");
        close(bip_socket);
        bip_socket = -1;
        return false;
    }
    
    printf("BACnet: UDP socket created and bound to port 0xBAC0\n");
    
    return true;
}

/**
 * Cleanup BACnet/IP  
 */
void bip_cleanup(void)
{
    /* Nothing to clean up in minimal implementation */
}

/* ===== Minimal BIP stubs for core functionality ===== */

/**
 * Get my BACnet/IP address
 */
void bip_get_my_address(BACNET_ADDRESS *my_address)
{
    if (my_address) {
        my_address->net = 0;  /* No network number for IP */
        my_address->len = 6;  /* 4 bytes for IP + 2 bytes for port */
        my_address->adr[0] = My_BIP_Address.address[0];
        my_address->adr[1] = My_BIP_Address.address[1];
        my_address->adr[2] = My_BIP_Address.address[2];
        my_address->adr[3] = My_BIP_Address.address[3];
        my_address->adr[4] = (My_BIP_Address.port >> 8) & 0xFF;
        my_address->adr[5] = My_BIP_Address.port & 0xFF;
    }
}

/**
 * Get BACnet/IP broadcast address
 */
void bip_get_broadcast_address(BACNET_ADDRESS *dest)
{
    if (dest) {
        dest->net = 0;  /* No network number for IP */
        dest->len = 6;  /* 4 bytes for IP + 2 bytes for port */
        dest->adr[0] = My_Broadcast_Address.address[0];
        dest->adr[1] = My_Broadcast_Address.address[1];
        dest->adr[2] = My_Broadcast_Address.address[2];
        dest->adr[3] = My_Broadcast_Address.address[3];
        dest->adr[4] = (My_Broadcast_Address.port >> 8) & 0xFF;
        dest->adr[5] = My_Broadcast_Address.port & 0xFF;
    }
}

/**
 * Set my BACnet/IP address
 */
bool bip_set_addr(const BACNET_IP_ADDRESS *addr)
{
    if (addr) {
        My_BIP_Address = *addr;
        return true;
    }
    return false;
}

/**
 * Set BACnet/IP broadcast address
 */
bool bip_set_broadcast_addr(const BACNET_IP_ADDRESS *addr)
{
    if (addr) {
        My_Broadcast_Address = *addr;
        return true;
    }
    return false;
}

/**
 * Send BACnet/IP PDU via UDP socket with proper BVLC wrapper
 */
int bip_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned int pdu_len)
{
    struct sockaddr_in addr;
    uint8_t tx_buffer[1502];  /* Max BACnet packet + BVLC header */
    int bvlc_len;
    int total_len;
    int bytes_sent = 0;
    uint8_t bvlc_function = BVLC_ORIGINAL_UNICAST_NPDU;  /* Default to unicast */
    
    if (bip_socket < 0 || pdu == NULL || pdu_len == 0) {
        printf("BACnet: bip_send_pdu() - invalid socket or PDU\n");
        return 0;
    }
    
    /* Parse destination address from BACnet address format */
    if (dest == NULL || dest->len != 6) {
        return 0;
    }
    
    /* Determine if this is a broadcast address (global or subnet broadcast) */
    if ((dest->adr[0] == 255 && dest->adr[1] == 255 && 
         dest->adr[2] == 255 && dest->adr[3] == 255) ||
        (dest->adr[0] == My_Broadcast_Address.address[0] &&
         dest->adr[1] == My_Broadcast_Address.address[1] &&
         dest->adr[2] == My_Broadcast_Address.address[2] &&
         dest->adr[3] == My_Broadcast_Address.address[3])) {
        bvlc_function = BVLC_ORIGINAL_BROADCAST_NPDU;
    }
    
    /* Encode BVLC header (4 bytes: type + function + length)
       BVLC Type: 0x81 (Original BACnet/IP)
       BVLC Function: 0x0a (Unicast) or 0x0b (Broadcast)
       Length: includes BVLC header (4 bytes) + NPDU + APDU */
    total_len = 4 + pdu_len;  /* BVLC header + PDU */
    
    bvlc_len = bvlc_encode_header(tx_buffer, sizeof(tx_buffer), 
                                   bvlc_function, total_len);
    
    if (bvlc_len != 4) {
        printf("BACnet: Failed to encode BVLC header\n");
        return 0;
    }
    
    /* Copy PDU after BVLC header */
    memcpy(&tx_buffer[4], pdu, pdu_len);
    
    /* Build socket address from BACnet address */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(
        ((uint32_t)dest->adr[0] << 24) |
        ((uint32_t)dest->adr[1] << 16) |
        ((uint32_t)dest->adr[2] << 8) |
        ((uint32_t)dest->adr[3])
    );
    addr.sin_port = htons(((uint16_t)dest->adr[4] << 8) | dest->adr[5]);
    
    /* Send UDP packet with BVLC wrapper */
    bytes_sent = sendto(bip_socket, (const char *)tx_buffer, total_len, 0, 
                       (struct sockaddr *)&addr, sizeof(addr));
    
    if (bytes_sent < 0) {
        return 0;
    }
    
    return bytes_sent;
}

/**
 * Send BACnet/IP MPDU (BVLC already encoded) to a B/IP address
 */
int bip_send_mpdu(
    const BACNET_IP_ADDRESS *dest, const uint8_t *mtu, uint16_t mtu_len)
{
    struct sockaddr_in addr;
    int bytes_sent = 0;

    if (bip_socket < 0 || dest == NULL || mtu == NULL || mtu_len == 0) {
        printf("BACnet: bip_send_mpdu() - invalid socket or MTU\n");
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(
        ((uint32_t)dest->address[0] << 24) |
        ((uint32_t)dest->address[1] << 16) |
        ((uint32_t)dest->address[2] << 8) |
        ((uint32_t)dest->address[3])
    );
    addr.sin_port = htons(dest->port);

    bytes_sent = sendto(bip_socket, (const char *)mtu, mtu_len, 0,
                        (struct sockaddr *)&addr, sizeof(addr));
    if (bytes_sent < 0) {
        printf("BACnet: bip_send_mpdu() sendto failed: %d\n", bytes_sent);
        return 0;
    }

    return bytes_sent;
}

/**
 * Receive a BACnet/IP PDU from UDP socket
 */
uint16_t bip_receive(
    BACNET_ADDRESS *src,
    uint8_t *pdu,
    uint16_t max_pdu,
    unsigned timeout)
{
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int received_bytes = 0;
    uint16_t pdu_len = 0;
    uint8_t bvlc_function = 0;
    uint16_t bvlc_length = 0;
    int offset = 0;
    
    if (bip_socket < 0 || pdu == NULL || src == NULL) {
        return 0;
    }
    
    /* Set socket to non-blocking for timeout support */
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    setsockopt(bip_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    /* Receive UDP packet */
    received_bytes = recvfrom(bip_socket, (char *)pdu, max_pdu, 0,
                             (struct sockaddr *)&from_addr, &from_len);
    
    if (received_bytes <= 0) {
        return 0;  /* No data or error */
    }
    
    /* Decode BVLC header (4 bytes minimum) */
    if (received_bytes < 4) {
        return 0;  /* Too short */
    }
    
    /* Check BVLC type (should be 0x81 for BACnet/IP) */
    if (pdu[0] != BVLL_TYPE_BACNET_IP) {
        return 0;  /* Wrong BVLC type */
    }
    
    bvlc_function = pdu[1];
    bvlc_length = (pdu[2] << 8) | pdu[3];
    
    /* Extract source address from socket */
    src->net = 0;  /* Local network */
    src->len = 6;  /* 4 bytes IP + 2 bytes port */
    uint32_t from_ip = ntohl(from_addr.sin_addr.s_addr);
    src->adr[0] = (from_ip >> 24) & 0xFF;
    src->adr[1] = (from_ip >> 16) & 0xFF;
    src->adr[2] = (from_ip >> 8) & 0xFF;
    src->adr[3] = from_ip & 0xFF;
    uint16_t from_port = ntohs(from_addr.sin_port);
    src->adr[4] = (from_port >> 8) & 0xFF;
    src->adr[5] = from_port & 0xFF;
    
    /* Process BVLC function */
    if (bvlc_function == BVLC_ORIGINAL_UNICAST_NPDU ||
        bvlc_function == BVLC_ORIGINAL_BROADCAST_NPDU) {
        /* Original unicast/broadcast NPDU - return NPDU without BVLC header */
        offset = 4;  /* Skip 4-byte BVLC header */
        pdu_len = received_bytes - offset;
        if (pdu_len > 0 && pdu_len <= max_pdu) {
            /* Move NPDU to start of buffer (remove BVLC header) */
            memmove(pdu, &pdu[offset], pdu_len);
            return pdu_len;
        }
    } else if (bvlc_function == BVLC_FORWARDED_NPDU) {
        /* Forwarded NPDU - skip BVLC header + 6 bytes original source */
        offset = 4 + 6;
        pdu_len = received_bytes - offset;
        if (pdu_len > 0 && pdu_len <= max_pdu && received_bytes >= offset) {
            /* Extract original source from forwarded message */
            src->adr[0] = pdu[4];
            src->adr[1] = pdu[5];
            src->adr[2] = pdu[6];
            src->adr[3] = pdu[7];
            src->adr[4] = pdu[8];
            src->adr[5] = pdu[9];
            /* Move NPDU to start of buffer */
            memmove(pdu, &pdu[offset], pdu_len);
            return pdu_len;
        }
    }
    /* Ignore other BVLC functions like register, read-bdt, etc. */
    
    return 0;
}


