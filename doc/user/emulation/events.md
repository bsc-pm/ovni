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
------------------ Ovni 1.0.0 (model=O) --------------------

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

----------------- nOS-V 1.0.0 (model=V) -------------------

VTc	Creates a new task (punctual event)
VTx	Task execute (enter task body)
VTe	Task end (exit task body)
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

VAr	Enters nosv_create()
VAR	Exits nosv_create()
VAd	Enters nosv_destroy()
VAD	Exits nosv_destroy()
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

----------------- NODES 1.0.0 (model=D) -------------------

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

----------------- Kernel 1.0.0 (model=K) -------------------

KCO	Is out of the CPU due to a context switch
KCI	Is back in the CPU due to a context switch

----------------- Nanos6 1.0.0 (model=6) -------------------

6Tc	Creates a new task
6Tx	Task execute
6Te	Task end
6Tp	Task pause
6Tr	Task resume

6Yc	Task type create (punctual event)

6C[	Begins creating a new task
6C]	Ends creating a new task

6S[	Enters the scheduler serving mode
6S]	Ends the scheduler serving mode
6Sa	Begins to submit a ready task via addReadyTask()
6SA	Ends submitting a ready task via addReadyTask()
6Sp	Begins to process ready tasks via processReadyTasks()
6SP	Ends processing ready taska via processReadyTasks()
6Sr	Receives a task from another thread (punctual event)
6Ss	Sends a task to another thread (punctual event)
6S@	Self-assigns itself a task (punctual event)

6W[	Begins the worker body loop, looking for tasks
6W]	Ends the worker body loop
6Wt	Begins handling a task via handleTask()
6WT	Ends handling a task via handleTask()
6Ww	Begins switching to another worker via switchTo()
6WW	Ends switching to another worker via switchTo()
6Wm	Begins migrating the CPU via migrate()
6WM	Ends migrating the CPU via migrate()
6Ws	Begins suspending the worker via suspend()
6WS	Ends suspending the worker via suspend()
6Wr	Begins resuming another worker via resume()
6WR	Ends resuming another worker via resume()
6Wg	Enters the sponge mode
6WG	Exits the sponge mode
6W*	Signals another thread to wake up (punctual event)

6Pp	Set progress state to Progressing
6Pr	Set progress state to Resting
6Pa	Set progress state to Absorbing

6U[	Starts to submit a task via submitTask()
6U]	Ends the submission of a task via submitTask()

6F[	Begins to spawn a function via spawnFunction()
6F]	Ends spawning a function

6t[	Begins running the task body
6t]	Ends running the task body

6O[	Begins running the task body as taskfor collaborator
6O]	Ends running the task body as taskfor collaborator

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

6Ma	Begins allocating memory
6MA	Ends allocating memory
6Mf	Begins freeing memory
6MF	Ends freeing memory

----------------- TAMPI 1.0.0 (model=T) -------------------

TCi Begins to issue a non-blocking communication operation
TCI Ends issuing a non-blocking communication operation

TGc Begins to check pending requests from the global array
TGC Ends checking pending requests from the global array

TLi Begins the library code at an API function
TLI Ends the library code at an API function
TLp Begins the library code at a polling function
TLP Ends the library code at a polling function

TQa Begins to add a ticket/requests to a queue
TQA Ends adding a ticket/requests to a queue
TQt Begins to transfer tickets/requests from queues to global array
TQT Ends transfering tickets/requests from queues to global array

TRc Begins to process a completed request
TRC Ends processing a completed request
TRt Begins to test a single request with MPI_Test
TRT Ends testing a single request with MPI_Test
TRa Begins to test several requests with MPI_Testall
TRA Ends testing several requests with MPI_Testall
TRs Begins to test several requests with MPI_Testsome
TRS Ends testing several requests with MPI_Testsome

TTc Begins to create a ticket linked to a set of requests and a task
TTC Ends creating a ticket linked to a set of requests and a task
TTw Begins to wait a ticket completion
TTW Ends waiting a ticket completion

----------------- MPI 1.0.0 (model=M) ----------------------

MUi Enters MPI_Init
MUI Exits MPI_Init
MUt Enters MPI_Init_thread
MUT Exits MPI_Init_thread
MUf Enters MPI_Finalize
MUF Exits MPI_Finalize

MW[ Enters MPI_Wait
MW] Exits MPI_Wait
MWa Enters MPI_Waitall
MWA Exits MPI_Waitall
MWy Enters MPI_Waitany
MWY Exits MPI_Waitany
MWs Enters MPI_Waitsome
MWS Exits MPI_Waitsome

MT[ Enters MPI_Test
MT] Exits MPI_Test
MTa Enters MPI_Testall
MTA Exits MPI_Testall
MTy Enters MPI_Testany
MTY Exits MPI_Testany
MTs Enters MPI_Testsome
MTS Exits MPI_Testsome

MS[ Enters MPI_Send
MS] Exits MPI_Send
MSb Enters MPI_Bsend
MSB Exits MPI_Bsend
MSr Enters MPI_Rsend
MSR Exits MPI_Rsend
MSs Enters MPI_Ssend
MSS Exits MPI_Ssend
MR[ Enters MPI_Recv
MR] Exits MPI_Recv
MRs Enters MPI_Sendrecv
MRS Exits MPI_Sendrecv
MRo Enters MPI_Sendrecv_replace
MRO Exits MPI_Sendrecv_replace

MAg Enters MPI_Allgather
MAG Exits MPI_Allgather
MAr Enters MPI_Allreduce
MAR Exits MPI_Allreduce
MAa Enters MPI_Alltoall
MAA Exits MPI_Alltoall
MCb Enters MPI_Barrier
MCB Exits MPI_Barrier
MCe Enters MPI_Exscan
MCE Exits MPI_Exscan
MCs Enters MPI_Scan
MCS Exits MPI_Scan
MDb Enters MPI_Bcast
MDB Exits MPI_Bcast
MDg Enters MPI_Gather
MDG Exits MPI_Gather
MDs Enters MPI_Scatter
MDS Exits MPI_Scatter
ME[ Enters MPI_Reduce
ME] Exits MPI_Reduce
MEs Enters MPI_Reduce_scatter
MES Exits MPI_Reduce_scatter
MEb Enters MPI_Reduce_scatter_block
MEB Exits MPI_Reduce_scatter_block

Ms[ Enters MPI_Isend
Ms] Exits MPI_Isend
Msb Enters MPI_Ibsend
MsB Exits MPI_Ibsend
Msr Enters MPI_Irsend
MsR Exits MPI_Irsend
Mss Enters MPI_Issend
MsS Exits MPI_Issend
Mr[ Enters MPI_Irecv
Mr] Exits MPI_Irecv
Mrs Enters MPI_Isendrecv
MrS Exits MPI_Isendrecv
Mro Enters MPI_Isendrecv_replace
MrO Exits MPI_Isendrecv_replace

Mag Enters MPI_Iallgather
MaG Exits MPI_Iallgather
Mar Enters MPI_Iallreduce
MaR Exits MPI_Iallreduce
Maa Enters MPI_Ialltoall
MaA Exits MPI_Ialltoall
Mcb Enters MPI_Ibarrier
McB Exits MPI_Ibarrier
Mce Enters MPI_Iexscan
McE Exits MPI_Iexscan
Mcs Enters MPI_Iscan
McS Exits MPI_Iscan
Mdb Enters MPI_Ibcast
MdB Exits MPI_Ibcast
Mdg Enters MPI_Igather
MdG Exits MPI_Igather
Mds Enters MPI_Iscatter
MdS Exits MPI_Iscatter
Me[ Enters MPI_Ireduce
Me] Exits MPI_Ireduce
Mes Enters MPI_Ireduce_scatter
MeS Exits MPI_Ireduce_scatter
Meb Enters MPI_Ireduce_scatter_block
MeB Exits MPI_Ireduce_scatter_block
```
