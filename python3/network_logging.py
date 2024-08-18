import scapy.all as scapy
import logging


def packet_handler(packet):
    logging.info(f"Packet captured: {packet.summary()}")

def main():
    logging.basicConfig(filename='network.log', level=logging.INFO, format='%(asctime)s %(message)s')
    scapy.sniff(prn=packet_handler)

if __name__ == "__main__":
    main()