// Copyright (C) Gaijin Games KFT.  All rights reserved.

// type, name, gauge (not resetted each cycle)
#define DEF_NS_COUNTERS                                                                           \
  _NS(RX_BYTES, rxBytes, false)                       /* received bytes */                        \
  _NS(TX_BYTES, txBytes, false)                       /* transferred bytes */                     \
  _NS(RX_PACKETS, rxPackets, false)                   /* received packets */                      \
  _NS(TX_PACKETS, txPackets, false)                   /* transferred packets */                   \
  _NS(RTT, rtt, true)                                 /* ping in ms */                            \
  _NS(ENET_RTT, enetRTT, true)                        /* enet ping in ms */                       \
  _NS(SERVER_TICK, serverTick, true)                  /* server tick in ms */                     \
  _NS(DEV_SERVER_TICK, devServerTick, true)           /* (dev) server tick in ms */               \
  _NS(ENET_PLOSS, enetPLOSS, true)                    /* enet packet loss permille */             \
  _NS(CTRL_PLOSS, ctrlPLOSS, true)                    /* controls packet loss permille */         \
  _NS(AAS_PROCESSED, aasProcessedCount, false)        /* number of AAS received and processed */  \
  _NS(AAS_DESYNCS, aasDesyncCount, false)             /* number of AAS state mispredictions */    \
  _NS(AAS_DESYNC_POS_DIFF, aasDesyncPosDiff, false)   /* AAS position difference on desync */     \
  _NS(PAAS_PROCESSED, paasProcessedCount, false)      /* number of PAAS received and processed */ \
  _NS(PAAS_DESYNCS, paasDesyncCount, false)           /* number of PAAS state mispredictions */   \
  _NS(PAAS_DESYNC_POS_DIFF, paasDesyncPosDiff, false) /* PAAS position difference on desync */
