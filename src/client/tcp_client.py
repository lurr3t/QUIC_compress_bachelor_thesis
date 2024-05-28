import argparse
import csv
import pickle
import socket
import ssl
import os
import datetime
import sys
import time

sys.path.append(os.path.dirname("/home/lurr3t/exjobb/src"))
from src.lib.Utils import Utils

CSV_PATH = "/home/lurr3t/exjobb/src/logs/tcp.csv"

def send_files(client_socket):
    files_directory = "/home/lurr3t/exjobb/payloads/dynamic"
    files = os.listdir(files_directory)

    # Send the number of files to the server
    client_socket.sendall(len(files).to_bytes(4, byteorder="big"))

    for file_name in files:
        file_path = os.path.join(files_directory, file_name)
        with open(file_path, "rb") as file:
            # Send the file name length and file name
            client_socket.sendall(len(file_name).to_bytes(4, byteorder="big"))
            client_socket.sendall(file_name.encode("utf-8"))

            # Send the file size
            file_size = os.path.getsize(file_path)
            client_socket.sendall(file_size.to_bytes(8, byteorder="big"))

            # Send the file data
            while True:
                chunk = file.read(4096)
                if not chunk:
                    break
                client_socket.sendall(chunk)
            print("Sent: " + file_name + " of size: " + str(file_size / 1000) + " kB")

# Function to clear the file contents
def clear_file(file_path):
    with open(file_path, 'w') as file:
        file.truncate(0)

# Function to append latency to the CSV file
def append_latency(latency):
    with open(CSV_PATH, 'a', newline='') as file:
        writer = csv.writer(file)
        writer.writerow([latency])


def main(host, certfile, keyfile, cafile):
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    context.load_cert_chain(certfile=certfile,
                            keyfile=keyfile)
    context.load_verify_locations(cafile=cafile)

    start_time = datetime.datetime.now()
    with socket.create_connection((host, 25565)) as s:
        with context.wrap_socket(s, server_hostname=host) as client_socket:
            print("TLS connection established: ")

            send_files(client_socket)

            # Receive end time from server
            end_time_bytes = client_socket.recv(1024)

            end_time = pickle.loads(end_time_bytes)

            # Calculate end-to-end latency
            delta = end_time - start_time
            latency = "{:.1f}".format(delta.total_seconds() * 1000)


            print(f"End-to-end latency: {latency} ms")


            # Close connection after receiving end time
            client_socket.close()
            return latency




if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Client for sending files over TLS')
    parser.add_argument('-host', help='Host IP address', required=True)
    parser.add_argument('-certfile', help='Path to certificate file', required=True)
    parser.add_argument('-keyfile', help='Path to key file', required=True)
    parser.add_argument('-cafile', help='Path to CA file', required=True)
    args = parser.parse_args()
    clear_file(CSV_PATH)


    episode_iterations = 10
    max_file_size_kb = 500
    min_file_size_kb = 500
    file_size_increment_kb = 500

    size_i = min_file_size_kb
    while size_i <= max_file_size_kb:
        # create test file
        ut = Utils()
        ut.create_file(size_i * 1000, "/home/lurr3t/exjobb/payloads/dynamic")

        episode_total_latency = 0
        for i in range(episode_iterations):
            episode_total_latency += float(main(args.host, args.certfile, args.keyfile, args.cafile))
        # calculating average
        append_latency(f"{size_i} Kb \t {episode_total_latency / episode_iterations}")
        print(f"{size_i} Kb \t {episode_total_latency / episode_iterations}")
        size_i += file_size_increment_kb
