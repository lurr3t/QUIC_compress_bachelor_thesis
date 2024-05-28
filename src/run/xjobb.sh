#!/bin/bash
# Is put into $PATH to simplify automation
# Use -s to run server and -c to run client
# add -l to run locally
# USAGE -q <quic> || -t <tcp> -l <local> -s <server> || -c <client>
while getopts "sclqt" opt; do
  case ${opt} in
    s )
      if [[ -n $local ]]; then
        echo "Local: Server: "
        if [[ -n $quic ]]; then
          echo "QUIC"
          #python3 /home/lurr3t/exjobb/src/server/quic_server.py -host localhost -certfile /home/lurr3t/exjobb/cert/local/server/local-server.pem -keyfile /home/lurr3t/exjobb/cert/local/server/local-server-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
          python3 /home/lurr3t/exjobb/src/run/run_experiment.py -s S -host localhost -certfile /home/lurr3t/exjobb/cert/local/server/local-server.pem -keyfile /home/lurr3t/exjobb/cert/local/server/local-server-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
        else
          echo "TCP"
          #python3 /home/lurr3t/exjobb/src/server/tcp_server.py -host localhost -certfile /home/lurr3t/exjobb/cert/local/server/local-server.pem -keyfile /home/lurr3t/exjobb/cert/local/server/local-server-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
          python3 /home/lurr3t/exjobb/src/run/run_experiment.py -s S -host localhost -certfile /home/lurr3t/exjobb/cert/local/server/local-server.pem -keyfile /home/lurr3t/exjobb/cert/local/server/local-server-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
        fi
      else
        echo "Emulator: Server: "
        if [[ -n $quic ]]; then
          echo "QUIC"
          #python3 /home/lurr3t/exjobb/src/server/quic_server.py -host 10.0.1.102 -certfile /home/lurr3t/exjobb/cert/leo-emulator/server/leo-emulator-server.pem -keyfile /home/lurr3t/exjobb/cert/leo-emulator/server/leo-emulator-server-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
          python3 /home/lurr3t/exjobb/src/run/run_experiment.py -s S -host 10.0.1.102 -certfile /home/lurr3t/exjobb/cert/leo-emulator/server/leo-emulator-server.pem -keyfile /home/lurr3t/exjobb/cert/leo-emulator/server/leo-emulator-server-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
        else
          echo "TCP"
          #python3 /home/lurr3t/exjobb/src/server/tcp_server.py -host 10.0.1.102 -certfile /home/lurr3t/exjobb/cert/leo-emulator/server/leo-emulator-server.pem -keyfile /home/lurr3t/exjobb/cert/leo-emulator/server/leo-emulator-server-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
          python3 /home/lurr3t/exjobb/src/run/run_experiment.py -s S -host 10.0.1.102 -certfile /home/lurr3t/exjobb/cert/leo-emulator/server/leo-emulator-server.pem -keyfile /home/lurr3t/exjobb/cert/leo-emulator/server/leo-emulator-server-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
        fi
      fi
      ;;
    c )
      if [[ -n $local ]]; then
        echo "Local: Client: "
        if [[ -n $quic ]]; then
          echo "QUIC"
          #python3 /home/lurr3t/exjobb/src/client/quic_client.py -host localhost -certfile /home/lurr3t/exjobb/cert/local/client/local-client.pem -keyfile /home/lurr3t/exjobb/cert/local/client/local-client-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
          python3 /home/lurr3t/exjobb/src/run/run_experiment.py -c C -host localhost -certfile /home/lurr3t/exjobb/cert/local/client/local-client.pem -keyfile /home/lurr3t/exjobb/cert/local/client/local-client-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
        else
          echo "TCP"
          #python3 /home/lurr3t/exjobb/src/client/tcp_client.py -host localhost -certfile /home/lurr3t/exjobb/cert/local/client/local-client.pem -keyfile /home/lurr3t/exjobb/cert/local/client/local-client-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
          python3 /home/lurr3t/exjobb/src/run/run_experiment.py -c C -host localhost -certfile /home/lurr3t/exjobb/cert/local/client/local-client.pem -keyfile /home/lurr3t/exjobb/cert/local/client/local-client-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
        fi
      else
        echo "Emulator: Client: "
        if [[ -n $quic ]]; then
          echo "QUIC"
          #python3 /home/lurr3t/exjobb/src/client/quic_client.py -host 10.0.1.102 -certfile /home/lurr3t/exjobb/cert/leo-emulator/client/leo-emulator-client.pem -keyfile /home/lurr3t/exjobb/cert/leo-emulator/client/leo-emulator-client-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
          python3 /home/lurr3t/exjobb/src/run/run_experiment.py -c C -host 10.0.1.102 -certfile /home/lurr3t/exjobb/cert/leo-emulator/client/leo-emulator-client.pem -keyfile /home/lurr3t/exjobb/cert/leo-emulator/client/leo-emulator-client-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
        else
          echo "TCP"
          #python3 /home/lurr3t/exjobb/src/client/tcp_client.py -host 10.0.1.102 -certfile /home/lurr3t/exjobb/cert/leo-emulator/client/leo-emulator-client.pem -keyfile /home/lurr3t/exjobb/cert/leo-emulator/client/leo-emulator-client-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
          python3 /home/lurr3t/exjobb/src/run/run_experiment.py -c C -host 10.0.1.102 -certfile /home/lurr3t/exjobb/cert/leo-emulator/client/leo-emulator-client.pem -keyfile /home/lurr3t/exjobb/cert/leo-emulator/client/leo-emulator-client-key.pem -cafile /home/lurr3t/exjobb/cert/cacerts.pem
        fi
      fi
      ;;
    l )
      local=true
      ;;
    q )
      quic=true
      ;;
    t )
      tcp=true
      ;;
    \? )
      echo "Invalid option: $OPTARG" 1>&2
      ;;
  esac
done