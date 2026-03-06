import time
import random

LANES = ["North", "South", "East", "West"]

BASE_GREEN = 5
MAX_GREEN = 20
YELLOW_TIME = 3
MAX_WAIT_LIMIT = 30
SCALE_FACTOR = 1.2

# State variables
waiting_time = {lane: 0 for lane in LANES}

def get_traffic_data():
    """
    Simulates sensor + camera fusion
    Returns:
        vehicle_count: number of vehicles per lane
        emergency_lane: lane with emergency vehicle (or None)
    """
    vehicle_count = {lane: random.randint(0, 15) for lane in LANES}
    emergency_lane = random.choice([None, None, None] + LANES)
    return vehicle_count, emergency_lane

def calculate_density_score(vehicle_count):
    density_score = {}
    for lane in LANES:
        density_score[lane] = vehicle_count[lane] * 2 + waiting_time[lane]
    return density_score

def select_lane(density_score):
    # Starvation prevention
    for lane in LANES:
        if waiting_time[lane] >= MAX_WAIT_LIMIT:
            return lane

    # Normal priority selection
    return max(density_score, key=density_score.get)

def calculate_green_time(score):
    return min(MAX_GREEN, BASE_GREEN + int(score * SCALE_FACTOR))

def run_signal_cycle():
    vehicle_count, emergency_lane = get_traffic_data()

    print("\n================ TRAFFIC STATUS ================")
    for lane in LANES:
        print(f"{lane}: Vehicles={vehicle_count[lane]}, Waiting={waiting_time[lane]}s")

    # Emergency override
    if emergency_lane:
        print(f"\n🚨 EMERGENCY detected at {emergency_lane}! OVERRIDING signals.")
        green_lane = emergency_lane
        green_time = MAX_GREEN
    else:
        density_score = calculate_density_score(vehicle_count)
        green_lane = select_lane(density_score)
        green_time = calculate_green_time(density_score[green_lane])

    print(f"\n🟢 GREEN -> {green_lane} ({green_time}s)")
    time.sleep(1)

    # Update waiting times
    for lane in LANES:
        if lane == green_lane:
            waiting_time[lane] = 0
        else:
            waiting_time[lane] += green_time

    time.sleep(green_time)

    print(f"🟡 YELLOW -> {green_lane}")
    time.sleep(YELLOW_TIME)

    print(f"🔴 RED -> {green_lane}")

def main():
    print("SMART TRAFFIC SIGNAL CONTROLLER STARTED\n")
    cycle = 1

    while True:
        print(f"\n=========== CYCLE {cycle} ===========")
        run_signal_cycle()
        cycle += 1

if __name__ == "__main__":
    main()
