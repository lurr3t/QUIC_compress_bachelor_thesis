# client.py
import argparse
import asyncio
import csv
import os
import pickle
import datetime
import sys


from aioquic.asyncio import connect
from aioquic.quic.configuration import QuicConfiguration


sys.path.append(os.path.dirname("/home/lurr3t/exjobb/src"))
from src.lib.Utils import Utils


CSV_PATH = "/home/lurr3t/exjobb/src/logs/quic.csv"

# Function to clear the file contents
def clear_file(file_path):
    with open(file_path, 'w') as file:
        file.truncate(0)

# Function to append latency to the CSV file
def append_latency(latency):
    with open(CSV_PATH, 'a', newline='') as file:
        writer = csv.writer(file)
        writer.writerow([latency])


async def send_files(reader, writer):
    files_dir = "/home/lurr3t/exjobb/payloads/dynamic"

    # goes through all files in dir
    for file_name in os.listdir(files_dir):
        file_path = os.path.join(files_dir, file_name)
        with open(file_path, 'rb') as file:
            while True:
                chunk = file.read(1024)
                if not chunk:
                    break
                writer.write(chunk)
            # Makes sure that the buffer isn't overloaded with data
            await writer.drain()
            print(f"Sent {file_name} ")
    writer.write_eof()
    print("All files sent.")

async def run_client(host, port, ca_cert_path):
    configuration = QuicConfiguration(is_client=True)
    configuration.load_verify_locations(cafile=ca_cert_path)

    start_time = datetime.datetime.now()
    async with connect(host, port, configuration=configuration) as protocol:
        print("Connecting to server...",end="")

        await protocol.wait_connected()
        print("Connected")
        reader, writer = await protocol.create_stream()
        print("Stream created")

        await send_files(reader, writer)

        server_time_bytes = 0
        while not reader.at_eof():
            server_time_bytes = await reader.read(1024)

        writer.write_eof()
        end_time = pickle.loads(server_time_bytes)

        # Calculate end-to-end latency
        delta = end_time - start_time
        latency = "{:.1f}".format(delta.total_seconds() * 1000)

        print(f"Start time: {start_time} \t End time: {end_time}")

        print(f"End-to-end latency: {latency} ms")

        #append_latency(latency)

        protocol.close()
        await protocol.wait_closed()
        print("Client: DONE")

        return latency

def main(host, port, cafile):
    return asyncio.run(run_client(host, port, cafile))

if __name__ == "__main__":


    print("QUIC client")
    parser = argparse.ArgumentParser(description='Client for sending files over QUIC')
    parser.add_argument('-host', help='Host IP address', required=True)
    parser.add_argument('-certfile', help='Path to certificate file', required=False)
    parser.add_argument('-keyfile', help='Path to key file', required=False)
    parser.add_argument('-cafile', help='Path to CA file', required=True)
    args = parser.parse_args()
    clear_file(CSV_PATH)

    episode_iterations = 50
    max_file_size_kb = 10000
    min_file_size_kb = 500
    file_size_increment_kb = 500

    size_i = min_file_size_kb
    while size_i <= max_file_size_kb:
        # create test file
        ut = Utils()
        ut.create_file(size_i * 1000, "/home/lurr3t/exjobb/payloads/dynamic")

        episode_total_latency = 0
        for i in range(episode_iterations):
            episode_total_latency += float(main(args.host, 25565, args.cafile))
        # calculating average
        append_latency(f"{size_i} Kb \t {episode_total_latency / episode_iterations}")
        print(f"{size_i} Kb \t {episode_total_latency / episode_iterations}")
        size_i += file_size_increment_kb
