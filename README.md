# Scheduler dApp for OpenAirInterface

This repository integrates a Python-based scheduler dApp with OpenAirInterface 5G gNB using the E3 protocol. The dApp enables runtime control of MAC layer scheduling policies and provides real-time per-UE statistics monitoring.

## Overview

The system consists of two main components:

- **OpenAirInterface 5G gNB** (`openairinterface5g/`): Enhanced with E3 agent support for runtime scheduler control
- **Scheduler dApp** (`dApp-library/`): Python-based application that monitors UE statistics and controls scheduling policies

The dApp communicates with the gNB via the E3 interface using ASN.1-encoded messages over POSIX IPC or ZMQ transport.

## Building OpenAirInterface with E3 Agent

Follow the official OpenAirInterface tutorials and OpenRanGym dApp tutorial for detailed build instructions:

- **OpenRanGym dApp Tutorial**: https://openrangym.com/tutorials/dapps-oai
- **Official OAI Documentation**: https://gitlab.eurecom.fr/oai/openairinterface5g/-/wikis/home

## Running the Scheduler dApp

### 1. Start OpenAirInterface gNB

Launch the gNB with your configuration file:

```bash
cd openairinterface5g/cmake_targets/ran_build/build
sudo ./nr-softmodem -O <config_file> --sa --rfsim
```

The E3 agent will automatically start and listen for dApp connections.

### 2. Start the Scheduler dApp

In a separate terminal:

```bash
cd dApp-library/examples
python3 scheduler_dapp.py
```

**What the dApp does:**
- Connects to the gNB via E3 protocol
- Subscribes to scheduler statistics (RAN function ID: 2)
- Displays real-time per-UE metrics:
  - RNTI (Radio Network Temporary Identifier)
  - Average throughput (Mbps)
  - BLER (Block Error Rate %)
  - Current MCS (Modulation and Coding Scheme)
  - Pending bytes in buffer (KB)
  - Resource blocks (RBs) allocated
  - Beam index
- Exposes REST API for policy control

### 3. Interactive Policy Control (Optional)

Control scheduling policies interactively:

```bash
cd dApp-library/examples
python3 interactive_control.py
```

**Available Commands:**
- `pf`, `proportional`, `fair` → Switch to Proportional Fair scheduler
- `rr`, `robin`, `round` → Switch to Round Robin scheduler
- `status` → Display current policy
- `intent <text>` → Intent-based policy selection (requires ChatGPT API key)

### 4. REST API Control (Optional)

**Get current policy:**
```bash
curl http://localhost:8080/policy
```

**Set scheduling policy:**
```bash
# Round Robin (policy=0)
curl -X POST http://localhost:8080/policy \
     -H "Content-Type: application/json" \
     -d '{"policy": 0}'

# Proportional Fair (policy=1)
curl -X POST http://localhost:8080/policy \
     -H "Content-Type: application/json" \
     -d '{"policy": 1}'
```

## Built-in Scheduling Policies

The system includes two default scheduling algorithms:

### 1. Round Robin (Policy ID: 0)
- Equal resource allocation across all UEs
- Circular scheduling pattern
- Ensures fairness regardless of channel conditions
- Implementation: [gNB_scheduler_dlsch.c](openairinterface5g/openair2/LAYER2/NR_MAC_gNB/gNB_scheduler_dlsch.c#L921)

### 2. Proportional Fair (Policy ID: 1)
- Allocates resources based on: `weight = tbs_per_rb / avg_throughput`
- Balances fairness and spectral efficiency
- Prioritizes UEs with good channel conditions relative to their past throughput
- Implementation: [gNB_scheduler_dlsch.c](openairinterface5g/openair2/LAYER2/NR_MAC_gNB/gNB_scheduler_dlsch.c#L795)

Both policies handle HARQ retransmissions with highest priority before scheduling new data.

## Implementing Custom Schedulers

Follow these steps to add a new scheduling policy to the system.

### Step 1: Implement the Scheduler Function (C Code)

**File:** [openairinterface5g/openair2/LAYER2/NR_MAC_gNB/gNB_scheduler_dlsch.c](openairinterface5g/openair2/LAYER2/NR_MAC_gNB/gNB_scheduler_dlsch.c)

Add your scheduler function with this signature:

```c
void nr_dl_my_new_scheduler(const nr_dl_sched_params_t *params,
                            const nr_dl_candidate_t *candidates,
                            nr_dl_alloc_t *allocs,
                            int n_candidates)
{
    // Your scheduling algorithm implementation
    
    // Input available:
    // - params->vrb_map: Virtual RB allocation map
    // - params->n_rb_avail: Available resource blocks
    // - params->beam_idx: Current beam index
    // - candidates[]: Array of UE candidates with metrics
    //   - candidates[i].UE: UE information structure
    //   - candidates[i].pending_bytes: Data waiting to transmit
    //   - candidates[i].avg_throughput: Historical throughput
    //   - candidates[i].bler: Block error rate
    //   - candidates[i].current_mcs: Current MCS
    //   - candidates[i].beam_index: Beam for this UE
    
    // Output required (populate allocs[] array):
    // - allocs[i].scheduled: true if UE is scheduled, false otherwise
    // - allocs[i].rbStart: Starting resource block index
    // - allocs[i].rbSize: Number of resource blocks allocated
    // - allocs[i].mcs: Modulation and coding scheme to use
    // - allocs[i].min_rbSize: Minimum RBs (for TBS calculation)
    
    // Example skeleton:
    for (int i = 0; i < n_candidates; i++) {
        if (candidates[i].is_retx) {
            // Handle HARQ retransmissions first (highest priority)
            continue;
        }
        
        // Your allocation logic here
        // Calculate priority, allocate RBs, set MCS, etc.
        
        if (/* allocation successful */) {
            allocs[i].scheduled = true;
            allocs[i].rbStart = /* calculated start */;
            allocs[i].rbSize = /* calculated size */;
            allocs[i].mcs = /* calculated MCS */;
            allocs[i].min_rbSize = allocs[i].rbSize;
        } else {
            allocs[i].scheduled = false;
        }
    }
}
```

**Reference existing implementations:**
- Round Robin: Line ~921 in [gNB_scheduler_dlsch.c](openairinterface5g/openair2/LAYER2/NR_MAC_gNB/gNB_scheduler_dlsch.c#L921)
- Proportional Fair: Line ~795 in [gNB_scheduler_dlsch.c](openairinterface5g/openair2/LAYER2/NR_MAC_gNB/gNB_scheduler_dlsch.c#L795)

### Step 2: Declare the Function

**File:** [openairinterface5g/openair2/LAYER2/NR_MAC_gNB/mac_proto.h](openairinterface5g/openair2/LAYER2/NR_MAC_gNB/mac_proto.h)

Add function declaration:

```c
void nr_dl_my_new_scheduler(const nr_dl_sched_params_t *params,
                            const nr_dl_candidate_t *candidates,
                            nr_dl_alloc_t *allocs,
                            int n_candidates);
```

### Step 3: Define Policy Constant

**File:** [openairinterface5g/openair2/E3AP/service_models/scheduler_sm/scheduler_sm.h](openairinterface5g/openair2/E3AP/service_models/scheduler_sm/scheduler_sm.h)

Add policy ID constant (use the next available integer):

```c
#define SCHEDULER_POLICY_MY_NEW_SCHEDULER 2
```

**Existing policy IDs:**
- `SCHEDULER_POLICY_ROUND_ROBIN` = 0
- `SCHEDULER_POLICY_PROPORTIONAL_FAIR` = 1

### Step 4: Update Policy Switching Logic

**File:** [openairinterface5g/openair2/LAYER2/NR_MAC_gNB/gNB_scheduler_dlsch.c](openairinterface5g/openair2/LAYER2/NR_MAC_gNB/gNB_scheduler_dlsch.c)

Locate the `nr_update_scheduler_policy()` function (around line 102) and add your policy case:

```c
void nr_update_scheduler_policy(gNB_MAC_INST *mac)
{
#ifdef E3_AGENT
    e3_sm_scheduler_control_t *ctrl = &mac->e3_sm_scheduler_control;
    
    pthread_mutex_lock(&ctrl->mutex);
    
    if (ctrl->policy_ready) {
        uint8_t requested_policy = ctrl->requested_policy;
        ctrl->policy_ready = 0;
        pthread_mutex_unlock(&ctrl->mutex);
        
        if (requested_policy == SCHEDULER_POLICY_ROUND_ROBIN) {
            mac->dl_sched_policy = nr_dl_round_robin;
            LOG_I(NR_MAC, "[SCHEDULER] Policy switch to Round Robin\n");
        } else if (requested_policy == SCHEDULER_POLICY_PROPORTIONAL_FAIR) {
            mac->dl_sched_policy = nr_dl_proportional_fair;
            LOG_I(NR_MAC, "[SCHEDULER] Policy switch to Proportional Fair\n");
        } else if (requested_policy == SCHEDULER_POLICY_MY_NEW_SCHEDULER) {
            mac->dl_sched_policy = nr_dl_my_new_scheduler;
            LOG_I(NR_MAC, "[SCHEDULER] Policy switch to My New Scheduler\n");
        } else {
            LOG_W(NR_MAC, "[SCHEDULER] Unknown policy %d requested\n", requested_policy);
        }
        
        ctrl->current_policy = requested_policy;
    } else {
        pthread_mutex_unlock(&ctrl->mutex);
    }
#endif
}
```

### Step 5: Update ASN.1 Schema (Both Sides)

Update the policy enumeration comment in the ASN.1 schema files to document your new policy:

**Files to modify:**
1. [dApp-library/src/scheduler/scheduler_sm_schema.asn](dApp-library/src/scheduler/scheduler_sm_schema.asn)
2. [openairinterface5g/openair2/E3AP/RAN_FUNCTION/E3SM/scheduler_sm_schema.asn](openairinterface5g/openair2/E3AP/RAN_FUNCTION/E3SM/scheduler_sm_schema.asn)

Update the comment above the `SchedulerPolicy` definition:

```asn1
-- Policy enumeration:
-- 0 = Round Robin
-- 1 = Proportional Fair
-- 2 = My New Scheduler
SchedulerPolicy ::= INTEGER (0..255)
```

**Note:** ASN.1 schema changes require rebuilding both OAI and the dApp library.

### Step 6: Update dApp Library (Python)

**File:** [dApp-library/src/scheduler/scheduler_dapp.py](dApp-library/src/scheduler/scheduler_dapp.py)

Add policy constant (around line 38):

```python
class SchedulerDapp:
    # Existing policies
    POLICY_ROUND_ROBIN = 0
    POLICY_PROPORTIONAL_FAIR = 1
    
    # Add your new policy
    POLICY_MY_NEW_SCHEDULER = 2
    
    POLICY_NAMES = {
        POLICY_ROUND_ROBIN: "Round Robin",
        POLICY_PROPORTIONAL_FAIR: "Proportional Fair",
        POLICY_MY_NEW_SCHEDULER: "My New Scheduler"
    }
```

Update policy validation in the Flask API route (around line 109):

```python
@app.route('/policy', methods=['POST'])
def set_policy():
    data = request.json
    policy = data.get('policy')
    
    if policy not in [self.POLICY_ROUND_ROBIN, 
                      self.POLICY_PROPORTIONAL_FAIR,
                      self.POLICY_MY_NEW_SCHEDULER]:
        return jsonify({'error': 'Invalid policy value'}), 400
    
    # ... rest of the function
```

### Step 7: Update Interactive Control (Optional)

**File:** [dApp-library/examples/interactive_control.py](dApp-library/examples/interactive_control.py)

Add command mapping for your new scheduler (around line 58):

```python
COMMANDS = {
    'pf': (1, 'Proportional Fair'),
    'proportional': (1, 'Proportional Fair'),
    'fair': (1, 'Proportional Fair'),
    'rr': (0, 'Round Robin'),
    'robin': (0, 'Round Robin'),
    'round': (0, 'Round Robin'),
    'mynew': (2, 'My New Scheduler'),  # Add your command
    'new': (2, 'My New Scheduler'),    # Add alias
    # ...
}
```

### Step 8: Rebuild and Test

**Rebuild OpenAirInterface:**
```bash
cd openairinterface5g
./cmake_targets/build_oai --e3-agent --gNB -c -C
```

**Rebuild dApp Library (if ASN.1 schema changed):**
```bash
cd dApp-library
hatch build
pip3 install --force-reinstall "dist/*.tar.gz[all]"
```

**Test your new scheduler:**
```bash
# Terminal 1: Start gNB
cd openairinterface5g/cmake_targets/ran_build/build
sudo ./nr-softmodem -O <config_file> --sa --rfsim

# Terminal 2: Start dApp
cd dApp-library/examples
python3 scheduler_dapp.py

# Terminal 3: Switch to your new policy
curl -X POST http://localhost:8080/policy \
     -H "Content-Type: application/json" \
     -d '{"policy": 2}'
```

Check the gNB logs for the policy switch confirmation:
```
[NR_MAC] [SCHEDULER] Policy switch to My New Scheduler
```

## Key Data Structures

Understanding these structures is essential for implementing schedulers:

### Scheduler Parameters (`nr_dl_sched_params_t`)
```c
typedef struct nr_dl_sched_params {
    gNB_MAC_INST *mac;          // MAC instance
    frame_t frame;              // Current frame number
    slot_t slot;                // Current slot number
    int beam_idx;               // Current beam index
    int max_num_ue;             // Maximum UEs to schedule
    uint16_t slbitmap;          // Symbol level bitmap
    uint16_t *vrb_map;          // Virtual RB allocation map
    int n_rb_avail;             // Available resource blocks
    int min_mcs;                // Minimum MCS allowed
    float bler_lower;           // Lower BLER threshold
    float bler_upper;           // Upper BLER threshold
} nr_dl_sched_params_t;
```

### Candidate UE (`nr_dl_candidate_t`)
```c
typedef struct nr_dl_candidate {
    NR_UE_info_t *UE;           // UE information structure
    bool is_retx;               // Is this a retransmission?
    int retx_harq_pid;          // HARQ process ID for retx
    int retx_rbSize;            // Required RBs for retx
    uint32_t pending_bytes;     // Bytes waiting in buffer
    float avg_throughput;       // Average throughput (Mbps)
    float bler;                 // Block error rate
    uint8_t current_mcs;        // Current MCS
    uint8_t max_mcs;            // Maximum allowed MCS
    uint16_t beam_index;        // Beam index for this UE
    // ... additional fields
} nr_dl_candidate_t;
```

### Allocation Decision (`nr_dl_alloc_t`)
```c
typedef struct nr_dl_alloc {
    bool scheduled;             // Was this UE scheduled?
    int rbStart;                // Starting resource block
    int rbSize;                 // Number of RBs allocated
    int min_rbSize;             // Minimum RBs (for TBS)
    uint8_t mcs;                // Modulation and coding scheme
} nr_dl_alloc_t;
```
