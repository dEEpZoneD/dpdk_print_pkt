#include <stdio.h>
#include <string.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_mbuf.h>

#define RTE_LOG(level, format, ...) printf(format, ##__VA_ARGS__)

#define BURST_SIZE 32  // Number of packets to retrieve per burst

int main(int argc, char *argv[]) {
    uint8_t nb_ports = rte_eth_dev_count();
    if (nb_ports < 1) {
        RTE_LOG(RTE_LOG_ERR, "Error: No Ethernet ports found\n");
        return -1;
    }

    uint16_t port_id = 0;  // Use the first port by default

    // Environment initialization
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        RTE_LOG(RTE_LOG_ERR, "Error: rte_eal_init failed: %d\n", ret);
        return -1;
    }

    // Port configuration
    const uint16_t nb_rx_desc = BURST_SIZE; // Number of receive descriptors
    const uint16_t nb_tx_desc = BURST_SIZE; // Number of transmit descriptors (not used in this example)
    struct rte_eth_conf dev_conf = RTE_ETH_CONF_DEFAULT();
    dev_conf.rxmode.mq_mode = RTE_ETH_MQ_MODE_NONE; // No multi-queue mode
    ret = rte_eth_dev_configure(port_id, nb_rx_desc, nb_tx_desc, &dev_conf);
    if (ret < 0) {
        RTE_LOG(RTE_LOG_ERR, "Error: rte_eth_dev_configure failed: %d\n", ret);
        return -1;
    }

    // Receive queue setup
    ret = rte_eth_rx_queue_setup(port_id, 0, nb_rx_desc, 0, NULL, NULL);
    if (ret < 0) {
        RTE_LOG(RTE_LOG_ERR, "Error: rte_eth_rx_queue_setup failed: %d\n", ret);
        return -1;
    }

    // Start the port
    ret = rte_eth_dev_start(port_id);
    if (ret < 0) {
        RTE_LOG(RTE_LOG_ERR, "Error: rte_eth_dev_start failed: %d\n", ret);
        return -1;
    }

    // Main loop to poll for packets
    struct rte_mbuf *rx_pkts[BURST_SIZE];
    uint64_t prev_tsc = rte_rdtsc(); // Timestamp for rate calculation
    uint32_t nb_rx = 0;
    while (1) {
        // Retrieve packets from the receive queue
        unsigned nb_burst = rte_eth_rx_burst(port_id, 0, rx_pkts, BURST_SIZE);

        // Process received packets
        for (unsigned i = 0; i < nb_burst; i++) {
            struct rte_mbuf *mbuf = rx_pkts[i];
            const void *data = rte_pktmbuf_get_contiguous_data(mbuf, 0); // Get packet data pointer

            // Print packet data (replace with your processing logic)
            printf("Packet %u: Length %u, Data: %.*s\n", nb_rx, mbuf->pkt_len,
                   (int)(mbuf->data_len - mbuf->data_off), (const char *)data);

            // Free the packet (optional, packets might be recycled)
            rte_pktmbuf_free(mbuf);
            nb_rx++;
        }

        // Calculate and print packet receive rate (optional)
        uint64_t curr_tsc = rte_rdtsc();
        uint64_t diff_tsc = curr_tsc - prev_tsc;
        if (diff_tsc > (RTE_GET_TSC_FREQ() * 5)) { // Print rate every 5 seconds
            double hz = RTE_GET_TSC_FREQ();
            printf("Packets received: %u, Rate: %.2