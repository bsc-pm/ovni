# Emulator events

This file contains an exhaustive list of events supported by the emulator.

- Punctual events don't produce a state transition.
- All events refer to the current thread.
- Descriptions must be kept short.

```txt
**********************************************************
Please keep this list synchronized with the emulator code!
**********************************************************

MCV = Model Category Value

------------------------------------------------------------
MCV	Description
--------------------- Ovni (model=O) -----------------------

OHC	Creates a new thread (punctual event)
OHx	Begins the execution
OHp	Pauses the execution
OHc	Enters the cooling state (about to be paused)
OHw	Enters the warming state (about to be running)
OHe	Ends the execution

OAs	Switches it's own affinity to the given CPU
OAr	Remotely switches the affinity of the given thread

OB.	Emits a burst event to measure latency

OU[	Enters a region which contain past events (HACK)
OU]	Exits the region of past events (HACK)

-------------------- nOS-V (model=V) ----------------------

VTc	Creates a new task (punctual event)
VTx	Task execute
VTe	Task end
VTp	Task pause
VTr	Task resume

VYc	Task type create (punctual event)

VSr	Receives a task from another thread (punctual event)
VSs	Sends a task to another thread (punctual event)
VS@	Self-assigns itself a task (punctual event)
VSh	Enters the hungry state, waiting for a task
VSf	Is no longer hungry
VS[	Enters the scheduler server mode
VS]	Ends the scheduler server mode

VU[	Starts to submit a task
VU]	Ends the submission of a task

VMa	Starts allocating memory
VMA	Ends allocating memory
VMf	Starts freeing memory
VMF	Ends freeing memory

VAs	Enters nosv_submit()
VAS	Exits nosv_submit()
VAp	Enters nosv_pause()
VAP	Exits nosv_pause()
VAy	Enters nosv_yield()
VAY	Exits nosv_yield()
VAw	Enters nosv_waitfor()
VAW	Exits nosv_waitfor()
VAc	Enters nosv_schedpoint()
VAC	Exits nosv_schedpoint()

VHa	Enters nosv_attach()
VHA	Exits nosv_detach()
VHw	Begins the execution as a worker
VHW	Ends the execution as a worker
VHd	Begins the execution as the delegate
VHD	Ends the execution as the delegate

-------------------- TAMPI (model=T) ----------------------

TS[	Enters MPI_Send()
TS]	Exits MPI_Send()

TR[	Enters MPI_Recv()
TR]	Exits MPI_Recv()

Ts[	Enters MPI_Isend()
Ts]	Exits MPI_Isend()

Tr[	Enters MPI_Irecv()
Tr]	Exits MPI_Irecv()

TV[	Enters MPI_Wait()
TV]	Exits MPI_Wait()

TW[	Enters MPI_Waitall()
TW]	Exits MPI_Waitall()

-------------------- OpenMP (model=M) ----------------------

MT[	Task begins
MT]	Task ends

MP]	Parallel region begins
MP[	Parallel region ends

-------------------- NODES (model=D) ----------------------

DR[	Begins the registration of a task's accesses
DR]	Ends the registration of a task's accesses

DU[	Begins the unregistration of a task's accesses
DU]	Ends the unregistration of a task's accesses

DW[	Enters a blocking condition (waiting for an If0 task)
DW]	Exits a blocking condition (waiting for an If0 task)

DI[	Begins the inline execution of an If0 task
DI]	Ends the inline execution of an If0 task

DT[	Enters a taskwait
DT]	Exits a taskwait

DC[	Begins the creation of a task
DC]	Ends the creation of a task

DS[	Begins the submit of a task
DS]	Ends the submit of a task

DP[	Begins the spawn of a function
DP]	Ends the spawn of a function

-------------------- Kernel (model=K) ----------------------

KCO	Is out of the CPU due to a context switch
KCI	Is back in the CPU due to a context switch

-------------------- Nanos6 (model=6) ----------------------

6Tc	Task creation begins
6TC	Task creation ends
6Tx	Task execute
6Te	Task end
6Tp	Task pause
6Tr	Task resume

6Yc	Task type create (punctual event)

6S[	Enters the scheduler serving mode
6S]	Ends the scheduler serving mode
6Sa	Begins to submit a ready task via addReadyTask()
6SA	Ends submitting a ready task via addReadyTask()
6Sr	Receives a task from another thread (punctual event)
6Ss	Sends a task to another thread (punctual event)
6S@	Self-assigns itself a task (punctual event)

6W[	Begins the worker body loop, looking for tasks
6W]	Ends the worker body loop
6Wt	Begins handling a task via handleTask()
6WT	Ends handling a task via handleTask()

6U[	Starts to submit a task via submitTask()
6U]	Ends the submission of a task via submitTask()

6F[	Begins to spawn a function via spawnFunction()
6F]	Ends spawning a function

6Dr	Begins the registration of a task's accesses
6DR	Ends the registration of a task's accesses
6Du	Begins the unregistration of a task's accesses
6DU	Ends the unregistration of a task's accesses

6Bb	Begins to block the current task via blockCurrentTask()
6BB	Ends blocking the current task via blockCurrentTask()
6Bu	Begins to unblock a task
6BU	Ends unblocking a task
6Bw	Enters taskWait()
6BW	Exits taskWait()
6Bf	Enters taskFor()
6BF	Exits taskFor()

6He	Sets itself as external thread
6HE	Unsets itself as external thread
6Hw	Sets itself as worker thread
6HW	Unsets itself as worker thread
6Hl	Sets itself as leader thread
6HL	Unsets itself as leader thread
6Hm	Sets itself as main thread
6HM	Unsets itself as main thread
```
