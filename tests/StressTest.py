import threading
import random
import time
import requests
from datetime import datetime, timedelta
from queue import Queue
import statistics

# Datos para generar reservas únicas
guest_names = [
    "Juan García", "Ana López", "Pedro Ruiz", "María Fernández", "Luis Gómez",
    "Carmen Díaz", "José Martínez", "Laura Sánchez", "Miguel Torres", "Paula Romero",
    "Antonio Sánchez", "Beatriz Montoya", "Carlos Herrera", "Diana Velázquez", "Enrique Navarro",
    "Francisca Morales", "Guillermo Castillo", "Helena Jiménez", "Ignacio Reyes", "Julieta Campos",
    "Kevin Vargas", "Lorena Fuentes", "Marcos Leiva", "Natalia Cortés", "Óscar Peña",
    "Patricia Búrgos", "Quique Mendez", "Rosa Acosta", "Sergio Dominguez", "Teresa Vega",
    "Ursulo Rivera", "Verónica Hinojosa", "Waldo Parra", "Xenia Cáceres", "Yasmine Sol"
]

guest_emails = [f"user{i}@example.com" for i in range(35)]
guest_phones = [f"+3461234567{i:02d}" for i in range(35)]
room_numbers = list(range(101, 136))

base_date = datetime(2026, 2, 1)
check_in_dates = [(base_date + timedelta(days=i)).strftime("%Y-%m-%d") for i in range(90)]
check_out_dates = [(base_date + timedelta(days=i+1)).strftime("%Y-%m-%d") for i in range(90)]

number_of_nights = [((i % 5) + 1) for i in range(90)]
price_per_night = [(100.0 + (i % 30) * 5) for i in range(90)]
payment_methods = ["credit_card", "cash", "paypal", "bank_transfer", "crypto"]
timestamps = [1707427200 + i*86400 for i in range(90)]

# Estadísticas globales
latencies = []
success_count = 0
error_count = 0
stats_lock = threading.Lock()

def create_payload():
    """gen a random json file content to make a request"""
    guest_idx = random.randint(0, 34)
    room_idx = random.randint(0, 34)
    date_idx = random.randint(0, 89)
    other_idx = random.randint(0, 89)
    
    return {
        "guest_name": guest_names[guest_idx],
        "guest_email": guest_emails[guest_idx],
        "guest_phone": guest_phones[guest_idx],
        "room_number": room_numbers[room_idx],
        "room_type": "Standard",
        "number_of_guests": 2,
        "check_in_date": check_in_dates[date_idx],
        "check_out_date": check_out_dates[date_idx],
        "number_of_nights": number_of_nights[other_idx],
        "price_per_night": price_per_night[other_idx],
        "total_price": price_per_night[other_idx] * number_of_nights[other_idx],
        "payment_method": payment_methods[other_idx % len(payment_methods)],
        "paid": True,
        "reservation_status": "confirmed",
        "special_requests": "None",
        "created_at": timestamps[other_idx],
        "updated_at": timestamps[other_idx]
    }

def worker(worker_id, work_queue):
    """Worker thread: consume requests from queue and send to the server"""
    global success_count, error_count
    
    url = "http://localhost:8080"
    headers = {"Content-Type": "application/json"}
    
    while True:
        # Obtener trabajo de la queue
        request_num = work_queue.get()
        if request_num is None:  # Señal de fin
            break
        
        payload = create_payload()
        
        try:
            start_time = time.time()
            response = requests.post(url, json=payload, headers=headers, timeout=5)
            latency = (time.time() - start_time) * 1000  # convertir a ms
            
            with stats_lock:
                latencies.append(latency)
                if response.status_code == 200:
                    success_count += 1
                else:
                    error_count += 1
            
            if request_num % 100 == 0:
                print(f"[Worker {worker_id}] Request {request_num}: {response.status_code} ({latency:.2f}ms)")
        
        except Exception as e:
            with stats_lock:
                error_count += 1
            print(f"[Worker {worker_id}] Error: {e}")
        
        finally:
            work_queue.task_done()

# ============ MAIN STRESS TEST ============

NUM_WORKERS = 4          # 4 threads
TOTAL_REQUESTS = 15000    # 15000 total request
REQUEST_TIMEOUT = 120     # Timeout for the total request

print(f"Starting Stress Test")
print(f"   - Workers: {NUM_WORKERS}")
print(f"   - Total requests: {TOTAL_REQUESTS}")
print(f"   - This means {TOTAL_REQUESTS // NUM_WORKERS} requests por worker\n")

# Crear queue de trabajo
work_queue = Queue()

# Llenar queue (los números son solo IDs)
for i in range(1, TOTAL_REQUESTS + 1):
    work_queue.put(i)

# Agregar señales de fin (una por worker)
for _ in range(NUM_WORKERS):
    work_queue.put(None)

# Recordar tiempo de inicio
start_time = time.time()

# Crear e iniciar workers
workers = []
for i in range(NUM_WORKERS):
    t = threading.Thread(target=worker, args=(i, work_queue))
    t.start()
    workers.append(t)

# Esperar a que terminen
for t in workers:
    t.join()

elapsed_time = time.time() - start_time

# ============ RESULTADOS ============

print("Results")
print(f"\nTotal time: {elapsed_time:.2f} s")
print(f"Succesfull Requests : {success_count}")
print(f"Failed Requests : {error_count}")
print(f"Throughput: {TOTAL_REQUESTS / elapsed_time:.2f} requests/sec")

if latencies:
    print(f"\nLatency (ms):")
    print(f"   - Mín:  {min(latencies):.2f} ms")
    print(f"   - Máx:  {max(latencies):.2f} ms")
    print(f"   - Prom: {statistics.mean(latencies):.2f} ms")
    print(f"   - Std:  {statistics.stdev(latencies) if len(latencies) > 1 else 0:.2f} ms")
    
    # Percentiles
    sorted_latencies = sorted(latencies)
    p50_idx = len(sorted_latencies) // 2
    p95_idx = int(len(sorted_latencies) * 0.95)
    p99_idx = int(len(sorted_latencies) * 0.99)
    
    print(f"   - p50: {sorted_latencies[p50_idx]:.2f} ms (50% requests)")
    print(f"   - p95: {sorted_latencies[p95_idx]:.2f} ms (95% requests)")
    print(f"   - p99: {sorted_latencies[p99_idx]:.2f} ms (99% requests)")

print("\n" + "="*60)