# Pass -q for QUIC and -t for TCP

trap 'trap - SIGTERM && kill 0' SIGINT SIGTERM EXIT

flag=''

while getopts "tq" opt; do
    case $opt in
        t) flag="t" ;;
        q) flag="q" ;;
        *) echo "Invalid option: -$OPTARG" >&2 ;;
    esac
done

# Make sure that pox runs beforehand
# python3 /home/lurr3t/Documents/exjobb_dependencies/pox/pox.py misc.learning_switch &

host="83.226.96.155"
username="lurr3t"
pw=$(cat /Users/ludwigfallstrom/.ssh/pw)

# Start emulator
#route_path="Starlink_SEA_NY_15_ISL_path.log"
route_path="Starlink_SD_NY_15_BP_path.log"
run_emulator="sudo -E python3 emulator.py $route_path"
emulator_path="/home/lurr3t/exjobb/LeoEM/emulation_stage/"
command="$run_emulator"
                          #locates correct directory     #runs emulator
ssh -X -t $username@$host "cd $emulator_path && echo $pw | sudo -E -S $command" &


# Run node commands
ssh -X -q $username@$host 'bash -s' < node_commands.sh $flag &


echo "Press Enter to exit"
read -p ""

# Kills xterm and emulator instances
ssh -t $username@$host "for pid in \$(ps -ef | awk '/emulator.py/ {print \$2}'); do echo $pw | sudo -S kill -9 \$pid; done"
killall xterm

# cleans up the emulator
cleanup_command="sudo mn -c"
ssh -X -t $username@$host "echo $pw | sudo -S $cleanup_command"

