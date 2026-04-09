/**
 * lwipopts.h — Configuração mínima do lwIP para FarmTech (Pico W)
 *
 * O Pico SDK v1.5.x exige que o projeto forneça este arquivo
 * para configurar o stack TCP/IP (lwIP) do WiFi CYW43.
 */
#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

/* ---- Configuração de threading ---- */
#define NO_SYS                      1       /* sem RTOS, modo poll */
#define LWIP_SOCKET                 0       /* sem API socket BSD */
#define LWIP_NETCONN                0       /* sem netconn (requer threads) */

/* ---- Recursos de rede ---- */
#define LWIP_TCP                    1       /* TCP necessário para HTTP */
#define LWIP_UDP                    1       /* UDP para DHCP */
#define LWIP_DHCP                   1       /* DHCP automático */
#define LWIP_DNS                    0       /* DNS desabilitado (não necessário) */
#define LWIP_ICMP                   1       /* ICMP para ping */
#define LWIP_ARP                    1       /* ARP necessário */
#define LWIP_NETIF_HOSTNAME         1       /* hostname para CYW43 */

/* ---- Tamanhos de buffer ---- */
#define MEM_SIZE                    4096    /* heap do lwIP */
#define MEMP_NUM_TCP_PCB            4       /* conexões TCP simultâneas */
#define MEMP_NUM_TCP_SEG            32      /* segmentos TCP em fila */
#define TCP_SND_QUEUELEN            16      /* fila de envio TCP */
#define MEMP_NUM_PBUF               16      /* pbufs de referência */
#define PBUF_POOL_SIZE              16      /* pool de pbufs de dados */
#define TCP_MSS                     1460    /* MSS padrão */
#define TCP_SND_BUF                 (4 * TCP_MSS)
#define TCP_WND                     (4 * TCP_MSS)

/* ---- Callbacks ---- */
#define LWIP_CALLBACK_API           1

/* ---- Diagnóstico (mínimo para produção) ---- */
#define LWIP_DEBUG                  0
#define LWIP_STATS                  0
#define LWIP_STATS_DISPLAY          0

/* ---- Pico W CYW43 específico ---- */
#define CYW43_LWIP                  1

/* ---- Checksum por software (sem offload no RP2040) ---- */
#define CHECKSUM_CHECK_IP           1
#define CHECKSUM_CHECK_UDP          1
#define CHECKSUM_CHECK_TCP          1
#define CHECKSUM_GEN_IP             1
#define CHECKSUM_GEN_UDP            1
#define CHECKSUM_GEN_TCP            1

#endif /* _LWIPOPTS_H */
