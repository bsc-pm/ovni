# Emulator events

This is a exhaustive list of the events recognized by the emulator.
Built on Jun 13 2024.

## Model nanos6

List of events for the model *nanos6* with identifier **`6`** at version `1.1.0`:
<dl>
<dt><a id="6Yc" href="#6Yc"><pre>6Yc+(u32 typeid, str label)</pre></a></dt>
<dd>creates task type %{typeid} with label &quot;%{label}&quot;</dd>
<dt><a id="6Tc" href="#6Tc"><pre>6Tc(u32 taskid, u32 typeid)</pre></a></dt>
<dd>creates task %{taskid} with type %{typeid}</dd>
<dt><a id="6Tx" href="#6Tx"><pre>6Tx(u32 taskid)</pre></a></dt>
<dd>executes the task %{taskid}</dd>
<dt><a id="6Te" href="#6Te"><pre>6Te(u32 taskid)</pre></a></dt>
<dd>ends the task %{taskid}</dd>
<dt><a id="6Tp" href="#6Tp"><pre>6Tp(u32 taskid)</pre></a></dt>
<dd>pauses the task %{taskid}</dd>
<dt><a id="6Tr" href="#6Tr"><pre>6Tr(u32 taskid)</pre></a></dt>
<dd>resumes the task %{taskid}</dd>
<dt><a id="6W[" href="#6W["><pre>6W[</pre></a></dt>
<dd>enters worker main loop, looking for tasks</dd>
<dt><a id="6W]" href="#6W]"><pre>6W]</pre></a></dt>
<dd>leaves worker main loop, looking for tasks</dd>
<dt><a id="6Wt" href="#6Wt"><pre>6Wt</pre></a></dt>
<dd>begins handling a task via handleTask()</dd>
<dt><a id="6WT" href="#6WT"><pre>6WT</pre></a></dt>
<dd>ceases handling a task via handleTask()</dd>
<dt><a id="6Ww" href="#6Ww"><pre>6Ww</pre></a></dt>
<dd>begins switching to another worker via switchTo()</dd>
<dt><a id="6WW" href="#6WW"><pre>6WW</pre></a></dt>
<dd>ceases switching to another worker via switchTo()</dd>
<dt><a id="6Wm" href="#6Wm"><pre>6Wm</pre></a></dt>
<dd>begins migrating the current worker to another CPU</dd>
<dt><a id="6WM" href="#6WM"><pre>6WM</pre></a></dt>
<dd>ceases migrating the current worker to another CPU</dd>
<dt><a id="6Ws" href="#6Ws"><pre>6Ws</pre></a></dt>
<dd>begins suspending the worker via suspend()</dd>
<dt><a id="6WS" href="#6WS"><pre>6WS</pre></a></dt>
<dd>ceases suspending the worker via suspend()</dd>
<dt><a id="6Wr" href="#6Wr"><pre>6Wr</pre></a></dt>
<dd>begins resuming another worker via resume()</dd>
<dt><a id="6WR" href="#6WR"><pre>6WR</pre></a></dt>
<dd>ceases resuming another worker via resume()</dd>
<dt><a id="6Wg" href="#6Wg"><pre>6Wg</pre></a></dt>
<dd>enters sponge mode (absorbing system noise)</dd>
<dt><a id="6WG" href="#6WG"><pre>6WG</pre></a></dt>
<dd>leaves sponge mode (absorbing system noise)</dd>
<dt><a id="6W*" href="#6W*"><pre>6W*</pre></a></dt>
<dd>signals another worker to wake up</dd>
<dt><a id="6Pp" href="#6Pp"><pre>6Pp</pre></a></dt>
<dd>sets progress state to Progressing</dd>
<dt><a id="6Pr" href="#6Pr"><pre>6Pr</pre></a></dt>
<dd>sets progress state to Resting</dd>
<dt><a id="6Pa" href="#6Pa"><pre>6Pa</pre></a></dt>
<dd>sets progress state to Absorbing</dd>
<dt><a id="6C[" href="#6C["><pre>6C[</pre></a></dt>
<dd>begins creating a new task</dd>
<dt><a id="6C]" href="#6C]"><pre>6C]</pre></a></dt>
<dd>ceases creating a new task</dd>
<dt><a id="6U[" href="#6U["><pre>6U[</pre></a></dt>
<dd>begins submitting a task via submitTask()</dd>
<dt><a id="6U]" href="#6U]"><pre>6U]</pre></a></dt>
<dd>ceases submitting a task via submitTask()</dd>
<dt><a id="6F[" href="#6F["><pre>6F[</pre></a></dt>
<dd>begins spawning a function via spawnFunction()</dd>
<dt><a id="6F]" href="#6F]"><pre>6F]</pre></a></dt>
<dd>ceases spawning a function via spawnFunction()</dd>
<dt><a id="6t[" href="#6t["><pre>6t[</pre></a></dt>
<dd>enters the task body</dd>
<dt><a id="6t]" href="#6t]"><pre>6t]</pre></a></dt>
<dd>leaves the task body</dd>
<dt><a id="6O[" href="#6O["><pre>6O[</pre></a></dt>
<dd>begins running the task body as taskfor collaborator</dd>
<dt><a id="6O]" href="#6O]"><pre>6O]</pre></a></dt>
<dd>ceases running the task body as taskfor collaborator</dd>
<dt><a id="6Ma" href="#6Ma"><pre>6Ma</pre></a></dt>
<dd>starts allocating memory</dd>
<dt><a id="6MA" href="#6MA"><pre>6MA</pre></a></dt>
<dd>stops  allocating memory</dd>
<dt><a id="6Mf" href="#6Mf"><pre>6Mf</pre></a></dt>
<dd>starts freeing memory</dd>
<dt><a id="6MF" href="#6MF"><pre>6MF</pre></a></dt>
<dd>stops  freeing memory</dd>
<dt><a id="6Dr" href="#6Dr"><pre>6Dr</pre></a></dt>
<dd>begins registration of task dependencies</dd>
<dt><a id="6DR" href="#6DR"><pre>6DR</pre></a></dt>
<dd>ceases registration of task dependencies</dd>
<dt><a id="6Du" href="#6Du"><pre>6Du</pre></a></dt>
<dd>begins unregistration of task dependencies</dd>
<dt><a id="6DU" href="#6DU"><pre>6DU</pre></a></dt>
<dd>ceases unregistration of task dependencies</dd>
<dt><a id="6S[" href="#6S["><pre>6S[</pre></a></dt>
<dd>begins scheduler serving mode</dd>
<dt><a id="6S]" href="#6S]"><pre>6S]</pre></a></dt>
<dd>ceases scheduler serving mode</dd>
<dt><a id="6Sa" href="#6Sa"><pre>6Sa</pre></a></dt>
<dd>begins submitting a ready task via addReadyTask()</dd>
<dt><a id="6SA" href="#6SA"><pre>6SA</pre></a></dt>
<dd>ceases submitting a ready task via addReadyTask()</dd>
<dt><a id="6Sp" href="#6Sp"><pre>6Sp</pre></a></dt>
<dd>begins processing ready tasks via processReadyTasks()</dd>
<dt><a id="6SP" href="#6SP"><pre>6SP</pre></a></dt>
<dd>ceases processing ready tasks via processReadyTasks()</dd>
<dt><a id="6S@" href="#6S@"><pre>6S@</pre></a></dt>
<dd>self assigns itself a task</dd>
<dt><a id="6Sr" href="#6Sr"><pre>6Sr</pre></a></dt>
<dd>receives a task from another thread</dd>
<dt><a id="6Ss" href="#6Ss"><pre>6Ss</pre></a></dt>
<dd>sends a task to another thread</dd>
<dt><a id="6Bb" href="#6Bb"><pre>6Bb</pre></a></dt>
<dd>begins blocking the current task</dd>
<dt><a id="6BB" href="#6BB"><pre>6BB</pre></a></dt>
<dd>ceases blocking the current task</dd>
<dt><a id="6Bu" href="#6Bu"><pre>6Bu</pre></a></dt>
<dd>begins unblocking a task</dd>
<dt><a id="6BU" href="#6BU"><pre>6BU</pre></a></dt>
<dd>ceases unblocking a task</dd>
<dt><a id="6Bw" href="#6Bw"><pre>6Bw</pre></a></dt>
<dd>enters a task wait</dd>
<dt><a id="6BW" href="#6BW"><pre>6BW</pre></a></dt>
<dd>leaves a task wait</dd>
<dt><a id="6Bf" href="#6Bf"><pre>6Bf</pre></a></dt>
<dd>enters a wait for</dd>
<dt><a id="6BF" href="#6BF"><pre>6BF</pre></a></dt>
<dd>leaves a wait for</dd>
<dt><a id="6He" href="#6He"><pre>6He</pre></a></dt>
<dd>begins execution as external thread</dd>
<dt><a id="6HE" href="#6HE"><pre>6HE</pre></a></dt>
<dd>ceases execution as external thread</dd>
<dt><a id="6Hw" href="#6Hw"><pre>6Hw</pre></a></dt>
<dd>begins execution as worker</dd>
<dt><a id="6HW" href="#6HW"><pre>6HW</pre></a></dt>
<dd>ceases execution as worker</dd>
<dt><a id="6Hl" href="#6Hl"><pre>6Hl</pre></a></dt>
<dd>begins execution as leader</dd>
<dt><a id="6HL" href="#6HL"><pre>6HL</pre></a></dt>
<dd>ceases execution as leader</dd>
<dt><a id="6Hm" href="#6Hm"><pre>6Hm</pre></a></dt>
<dd>begins execution as main thread</dd>
<dt><a id="6HM" href="#6HM"><pre>6HM</pre></a></dt>
<dd>ceases execution as main thread</dd>
</dl>

## Model nodes

List of events for the model *nodes* with identifier **`D`** at version `1.0.0`:
<dl>
<dt><a id="DR[" href="#DR["><pre>DR[</pre></a></dt>
<dd>begins registering task accesses</dd>
<dt><a id="DR]" href="#DR]"><pre>DR]</pre></a></dt>
<dd>ceases registering task accesses</dd>
<dt><a id="DU[" href="#DU["><pre>DU[</pre></a></dt>
<dd>begins unregistering task accesses</dd>
<dt><a id="DU]" href="#DU]"><pre>DU]</pre></a></dt>
<dd>ceases unregistering task accesses</dd>
<dt><a id="DW[" href="#DW["><pre>DW[</pre></a></dt>
<dd>enters a blocking condition (waiting for an If0 task)</dd>
<dt><a id="DW]" href="#DW]"><pre>DW]</pre></a></dt>
<dd>leaves a blocking condition (waiting for an If0 task)</dd>
<dt><a id="DI[" href="#DI["><pre>DI[</pre></a></dt>
<dd>begins the inline execution of an If0 task</dd>
<dt><a id="DI]" href="#DI]"><pre>DI]</pre></a></dt>
<dd>ceases the inline execution of an If0 task</dd>
<dt><a id="DT[" href="#DT["><pre>DT[</pre></a></dt>
<dd>enters a taskwait</dd>
<dt><a id="DT]" href="#DT]"><pre>DT]</pre></a></dt>
<dd>leaves a taskwait</dd>
<dt><a id="DC[" href="#DC["><pre>DC[</pre></a></dt>
<dd>begins creating a task</dd>
<dt><a id="DC]" href="#DC]"><pre>DC]</pre></a></dt>
<dd>ceases creating a task</dd>
<dt><a id="DS[" href="#DS["><pre>DS[</pre></a></dt>
<dd>begins submitting a task</dd>
<dt><a id="DS]" href="#DS]"><pre>DS]</pre></a></dt>
<dd>ceases submitting a task</dd>
<dt><a id="DP[" href="#DP["><pre>DP[</pre></a></dt>
<dd>begins spawning a function</dd>
<dt><a id="DP]" href="#DP]"><pre>DP]</pre></a></dt>
<dd>ceases spawning a function</dd>
</dl>

## Model kernel

List of events for the model *kernel* with identifier **`K`** at version `1.0.0`:
<dl>
<dt><a id="KCO" href="#KCO"><pre>KCO</pre></a></dt>
<dd>out of CPU</dd>
<dt><a id="KCI" href="#KCI"><pre>KCI</pre></a></dt>
<dd>back to CPU</dd>
</dl>

## Model mpi

List of events for the model *mpi* with identifier **`M`** at version `1.0.0`:
<dl>
<dt><a id="MUf" href="#MUf"><pre>MUf</pre></a></dt>
<dd>enters MPI_Finalize()</dd>
<dt><a id="MUF" href="#MUF"><pre>MUF</pre></a></dt>
<dd>leaves MPI_Finalize()</dd>
<dt><a id="MUi" href="#MUi"><pre>MUi</pre></a></dt>
<dd>enters MPI_Init()</dd>
<dt><a id="MUI" href="#MUI"><pre>MUI</pre></a></dt>
<dd>leaves MPI_Init()</dd>
<dt><a id="MUt" href="#MUt"><pre>MUt</pre></a></dt>
<dd>enters MPI_Init_thread()</dd>
<dt><a id="MUT" href="#MUT"><pre>MUT</pre></a></dt>
<dd>leaves MPI_Init_thread()</dd>
<dt><a id="MW[" href="#MW["><pre>MW[</pre></a></dt>
<dd>enters MPI_Wait()</dd>
<dt><a id="MW]" href="#MW]"><pre>MW]</pre></a></dt>
<dd>leaves MPI_Wait()</dd>
<dt><a id="MWa" href="#MWa"><pre>MWa</pre></a></dt>
<dd>enters MPI_Waitall()</dd>
<dt><a id="MWA" href="#MWA"><pre>MWA</pre></a></dt>
<dd>leaves MPI_Waitall()</dd>
<dt><a id="MWs" href="#MWs"><pre>MWs</pre></a></dt>
<dd>enters MPI_Waitsome()</dd>
<dt><a id="MWS" href="#MWS"><pre>MWS</pre></a></dt>
<dd>leaves MPI_Waitsome()</dd>
<dt><a id="MWy" href="#MWy"><pre>MWy</pre></a></dt>
<dd>enters MPI_Waitany()</dd>
<dt><a id="MWY" href="#MWY"><pre>MWY</pre></a></dt>
<dd>leaves MPI_Waitany()</dd>
<dt><a id="MT[" href="#MT["><pre>MT[</pre></a></dt>
<dd>enters MPI_Test()</dd>
<dt><a id="MT]" href="#MT]"><pre>MT]</pre></a></dt>
<dd>leaves MPI_Test()</dd>
<dt><a id="MTa" href="#MTa"><pre>MTa</pre></a></dt>
<dd>enters MPI_Testall()</dd>
<dt><a id="MTA" href="#MTA"><pre>MTA</pre></a></dt>
<dd>leaves MPI_Testall()</dd>
<dt><a id="MTy" href="#MTy"><pre>MTy</pre></a></dt>
<dd>enters MPI_Testany()</dd>
<dt><a id="MTY" href="#MTY"><pre>MTY</pre></a></dt>
<dd>leaves MPI_Testany()</dd>
<dt><a id="MTs" href="#MTs"><pre>MTs</pre></a></dt>
<dd>enters MPI_Testsome()</dd>
<dt><a id="MTS" href="#MTS"><pre>MTS</pre></a></dt>
<dd>leaves MPI_Testsome()</dd>
<dt><a id="MS[" href="#MS["><pre>MS[</pre></a></dt>
<dd>enters MPI_Send()</dd>
<dt><a id="MS]" href="#MS]"><pre>MS]</pre></a></dt>
<dd>leaves MPI_Send()</dd>
<dt><a id="MSb" href="#MSb"><pre>MSb</pre></a></dt>
<dd>enters MPI_Bsend()</dd>
<dt><a id="MSB" href="#MSB"><pre>MSB</pre></a></dt>
<dd>leaves MPI_Bsend()</dd>
<dt><a id="MSr" href="#MSr"><pre>MSr</pre></a></dt>
<dd>enters MPI_Rsend()</dd>
<dt><a id="MSR" href="#MSR"><pre>MSR</pre></a></dt>
<dd>leaves MPI_Rsend()</dd>
<dt><a id="MSs" href="#MSs"><pre>MSs</pre></a></dt>
<dd>enters MPI_Ssend()</dd>
<dt><a id="MSS" href="#MSS"><pre>MSS</pre></a></dt>
<dd>leaves MPI_Ssend()</dd>
<dt><a id="MR[" href="#MR["><pre>MR[</pre></a></dt>
<dd>enters MPI_Recv()</dd>
<dt><a id="MR]" href="#MR]"><pre>MR]</pre></a></dt>
<dd>leaves MPI_Recv()</dd>
<dt><a id="MRs" href="#MRs"><pre>MRs</pre></a></dt>
<dd>enters MPI_Sendrecv()</dd>
<dt><a id="MRS" href="#MRS"><pre>MRS</pre></a></dt>
<dd>leaves MPI_Sendrecv()</dd>
<dt><a id="MRo" href="#MRo"><pre>MRo</pre></a></dt>
<dd>enters MPI_Sendrecv_replace()</dd>
<dt><a id="MRO" href="#MRO"><pre>MRO</pre></a></dt>
<dd>leaves MPI_Sendrecv_replace()</dd>
<dt><a id="MAg" href="#MAg"><pre>MAg</pre></a></dt>
<dd>enters MPI_Allgather()</dd>
<dt><a id="MAG" href="#MAG"><pre>MAG</pre></a></dt>
<dd>leaves MPI_Allgather()</dd>
<dt><a id="MAr" href="#MAr"><pre>MAr</pre></a></dt>
<dd>enters MPI_Allreduce()</dd>
<dt><a id="MAR" href="#MAR"><pre>MAR</pre></a></dt>
<dd>leaves MPI_Allreduce()</dd>
<dt><a id="MAa" href="#MAa"><pre>MAa</pre></a></dt>
<dd>enters MPI_Alltoall()</dd>
<dt><a id="MAA" href="#MAA"><pre>MAA</pre></a></dt>
<dd>leaves MPI_Alltoall()</dd>
<dt><a id="MCb" href="#MCb"><pre>MCb</pre></a></dt>
<dd>enters MPI_Barrier()</dd>
<dt><a id="MCB" href="#MCB"><pre>MCB</pre></a></dt>
<dd>leaves MPI_Barrier()</dd>
<dt><a id="MCe" href="#MCe"><pre>MCe</pre></a></dt>
<dd>enters MPI_Exscan()</dd>
<dt><a id="MCE" href="#MCE"><pre>MCE</pre></a></dt>
<dd>leaves MPI_Exscan()</dd>
<dt><a id="MCs" href="#MCs"><pre>MCs</pre></a></dt>
<dd>enters MPI_Scan()</dd>
<dt><a id="MCS" href="#MCS"><pre>MCS</pre></a></dt>
<dd>leaves MPI_Scan()</dd>
<dt><a id="MDb" href="#MDb"><pre>MDb</pre></a></dt>
<dd>enters MPI_Bcast()</dd>
<dt><a id="MDB" href="#MDB"><pre>MDB</pre></a></dt>
<dd>leaves MPI_Bcast()</dd>
<dt><a id="MDg" href="#MDg"><pre>MDg</pre></a></dt>
<dd>enters MPI_Gather()</dd>
<dt><a id="MDG" href="#MDG"><pre>MDG</pre></a></dt>
<dd>leaves MPI_Gather()</dd>
<dt><a id="MDs" href="#MDs"><pre>MDs</pre></a></dt>
<dd>enters MPI_Scatter()</dd>
<dt><a id="MDS" href="#MDS"><pre>MDS</pre></a></dt>
<dd>leaves MPI_Scatter()</dd>
<dt><a id="ME[" href="#ME["><pre>ME[</pre></a></dt>
<dd>enters MPI_Reduce()</dd>
<dt><a id="ME]" href="#ME]"><pre>ME]</pre></a></dt>
<dd>leaves MPI_Reduce()</dd>
<dt><a id="MEs" href="#MEs"><pre>MEs</pre></a></dt>
<dd>enters MPI_Reduce_scatter()</dd>
<dt><a id="MES" href="#MES"><pre>MES</pre></a></dt>
<dd>leaves MPI_Reduce_scatter()</dd>
<dt><a id="MEb" href="#MEb"><pre>MEb</pre></a></dt>
<dd>enters MPI_Reduce_scatter_block()</dd>
<dt><a id="MEB" href="#MEB"><pre>MEB</pre></a></dt>
<dd>leaves MPI_Reduce_scatter_block()</dd>
<dt><a id="Ms[" href="#Ms["><pre>Ms[</pre></a></dt>
<dd>enters MPI_Isend()</dd>
<dt><a id="Ms]" href="#Ms]"><pre>Ms]</pre></a></dt>
<dd>leaves MPI_Isend()</dd>
<dt><a id="Msb" href="#Msb"><pre>Msb</pre></a></dt>
<dd>enters MPI_Ibsend()</dd>
<dt><a id="MsB" href="#MsB"><pre>MsB</pre></a></dt>
<dd>leaves MPI_Ibsend()</dd>
<dt><a id="Msr" href="#Msr"><pre>Msr</pre></a></dt>
<dd>enters MPI_Irsend()</dd>
<dt><a id="MsR" href="#MsR"><pre>MsR</pre></a></dt>
<dd>leaves MPI_Irsend()</dd>
<dt><a id="Mss" href="#Mss"><pre>Mss</pre></a></dt>
<dd>enters MPI_Issend()</dd>
<dt><a id="MsS" href="#MsS"><pre>MsS</pre></a></dt>
<dd>leaves MPI_Issend()</dd>
<dt><a id="Mr[" href="#Mr["><pre>Mr[</pre></a></dt>
<dd>enters MPI_Irecv()</dd>
<dt><a id="Mr]" href="#Mr]"><pre>Mr]</pre></a></dt>
<dd>leaves MPI_Irecv()</dd>
<dt><a id="Mrs" href="#Mrs"><pre>Mrs</pre></a></dt>
<dd>enters MPI_Isendrecv()</dd>
<dt><a id="MrS" href="#MrS"><pre>MrS</pre></a></dt>
<dd>leaves MPI_Isendrecv()</dd>
<dt><a id="Mro" href="#Mro"><pre>Mro</pre></a></dt>
<dd>enters MPI_Isendrecv_replace()</dd>
<dt><a id="MrO" href="#MrO"><pre>MrO</pre></a></dt>
<dd>leaves MPI_Isendrecv_replace()</dd>
<dt><a id="Mag" href="#Mag"><pre>Mag</pre></a></dt>
<dd>enters MPI_Iallgather()</dd>
<dt><a id="MaG" href="#MaG"><pre>MaG</pre></a></dt>
<dd>leaves MPI_Iallgather()</dd>
<dt><a id="Mar" href="#Mar"><pre>Mar</pre></a></dt>
<dd>enters MPI_Iallreduce()</dd>
<dt><a id="MaR" href="#MaR"><pre>MaR</pre></a></dt>
<dd>leaves MPI_Iallreduce()</dd>
<dt><a id="Maa" href="#Maa"><pre>Maa</pre></a></dt>
<dd>enters MPI_Ialltoall()</dd>
<dt><a id="MaA" href="#MaA"><pre>MaA</pre></a></dt>
<dd>leaves MPI_Ialltoall()</dd>
<dt><a id="Mcb" href="#Mcb"><pre>Mcb</pre></a></dt>
<dd>enters MPI_Ibarrier()</dd>
<dt><a id="McB" href="#McB"><pre>McB</pre></a></dt>
<dd>leaves MPI_Ibarrier()</dd>
<dt><a id="Mce" href="#Mce"><pre>Mce</pre></a></dt>
<dd>enters MPI_Iexscan()</dd>
<dt><a id="McE" href="#McE"><pre>McE</pre></a></dt>
<dd>leaves MPI_Iexscan()</dd>
<dt><a id="Mcs" href="#Mcs"><pre>Mcs</pre></a></dt>
<dd>enters MPI_Iscan()</dd>
<dt><a id="McS" href="#McS"><pre>McS</pre></a></dt>
<dd>leaves MPI_Iscan()</dd>
<dt><a id="Mdb" href="#Mdb"><pre>Mdb</pre></a></dt>
<dd>enters MPI_Ibcast()</dd>
<dt><a id="MdB" href="#MdB"><pre>MdB</pre></a></dt>
<dd>leaves MPI_Ibcast()</dd>
<dt><a id="Mdg" href="#Mdg"><pre>Mdg</pre></a></dt>
<dd>enters MPI_Igather()</dd>
<dt><a id="MdG" href="#MdG"><pre>MdG</pre></a></dt>
<dd>leaves MPI_Igather()</dd>
<dt><a id="Mds" href="#Mds"><pre>Mds</pre></a></dt>
<dd>enters MPI_Iscatter()</dd>
<dt><a id="MdS" href="#MdS"><pre>MdS</pre></a></dt>
<dd>leaves MPI_Iscatter()</dd>
<dt><a id="Me[" href="#Me["><pre>Me[</pre></a></dt>
<dd>enters MPI_Ireduce()</dd>
<dt><a id="Me]" href="#Me]"><pre>Me]</pre></a></dt>
<dd>leaves MPI_Ireduce()</dd>
<dt><a id="Mes" href="#Mes"><pre>Mes</pre></a></dt>
<dd>enters MPI_Ireduce_scatter()</dd>
<dt><a id="MeS" href="#MeS"><pre>MeS</pre></a></dt>
<dd>leaves MPI_Ireduce_scatter()</dd>
<dt><a id="Meb" href="#Meb"><pre>Meb</pre></a></dt>
<dd>enters MPI_Ireduce_scatter_block()</dd>
<dt><a id="MeB" href="#MeB"><pre>MeB</pre></a></dt>
<dd>leaves MPI_Ireduce_scatter_block()</dd>
</dl>

## Model ovni

List of events for the model *ovni* with identifier **`O`** at version `1.0.0`:
<dl>
<dt><a id="OAr" href="#OAr"><pre>OAr(i32 cpu, i32 tid)</pre></a></dt>
<dd>changes the affinity of thread %{tid} to CPU %{cpu}</dd>
<dt><a id="OAs" href="#OAs"><pre>OAs(i32 cpu)</pre></a></dt>
<dd>switches it&apos;s own affinity to the CPU %{cpu}</dd>
<dt><a id="OB." href="#OB."><pre>OB.</pre></a></dt>
<dd>emits a burst event to measure latency</dd>
<dt><a id="OHC" href="#OHC"><pre>OHC(i32 cpu, u64 tag)</pre></a></dt>
<dd>creates a new thread on CPU %{cpu} with tag %#llx{tag}</dd>
<dt><a id="OHc" href="#OHc"><pre>OHc</pre></a></dt>
<dd>enters the Cooling state (about to be paused)</dd>
<dt><a id="OHe" href="#OHe"><pre>OHe</pre></a></dt>
<dd>ends the execution</dd>
<dt><a id="OHp" href="#OHp"><pre>OHp</pre></a></dt>
<dd>pauses the execution</dd>
<dt><a id="OHr" href="#OHr"><pre>OHr</pre></a></dt>
<dd>resumes the execution</dd>
<dt><a id="OHw" href="#OHw"><pre>OHw</pre></a></dt>
<dd>enters the Warming state (about to be running)</dd>
<dt><a id="OHx" href="#OHx"><pre>OHx(i32 cpu, i32 tid, u64 tag)</pre></a></dt>
<dd>begins the execution on CPU %{cpu} created from %{tid} with tag %#llx{tag}</dd>
<dt><a id="OCn" href="#OCn"><pre>OCn(i32 cpu)</pre></a></dt>
<dd>informs there are %{cpu} CPUs</dd>
<dt><a id="OF[" href="#OF["><pre>OF[</pre></a></dt>
<dd>begins flushing events to disk</dd>
<dt><a id="OF]" href="#OF]"><pre>OF]</pre></a></dt>
<dd>ceases flushing events to disk</dd>
<dt><a id="OU[" href="#OU["><pre>OU[</pre></a></dt>
<dd>enters unordered event region</dd>
<dt><a id="OU]" href="#OU]"><pre>OU]</pre></a></dt>
<dd>leaves unordered event region</dd>
</dl>

## Model openmp

List of events for the model *openmp* with identifier **`P`** at version `1.1.0`:
<dl>
<dt><a id="PBb" href="#PBb"><pre>PBb</pre></a></dt>
<dd>begins plain barrier</dd>
<dt><a id="PBB" href="#PBB"><pre>PBB</pre></a></dt>
<dd>ceases plain barrier</dd>
<dt><a id="PBj" href="#PBj"><pre>PBj</pre></a></dt>
<dd>begins join barrier</dd>
<dt><a id="PBJ" href="#PBJ"><pre>PBJ</pre></a></dt>
<dd>ceases join barrier</dd>
<dt><a id="PBf" href="#PBf"><pre>PBf</pre></a></dt>
<dd>begins fork barrier</dd>
<dt><a id="PBF" href="#PBF"><pre>PBF</pre></a></dt>
<dd>ceases fork barrier</dd>
<dt><a id="PBt" href="#PBt"><pre>PBt</pre></a></dt>
<dd>begins tasking barrier</dd>
<dt><a id="PBT" href="#PBT"><pre>PBT</pre></a></dt>
<dd>ceases tasking barrier</dd>
<dt><a id="PBs" href="#PBs"><pre>PBs</pre></a></dt>
<dd>begins spin wait</dd>
<dt><a id="PBS" href="#PBS"><pre>PBS</pre></a></dt>
<dd>ceases spin wait</dd>
<dt><a id="PIa" href="#PIa"><pre>PIa</pre></a></dt>
<dd>begins critical acquiring</dd>
<dt><a id="PIA" href="#PIA"><pre>PIA</pre></a></dt>
<dd>ceases critical acquiring</dd>
<dt><a id="PIr" href="#PIr"><pre>PIr</pre></a></dt>
<dd>begins critical releasing</dd>
<dt><a id="PIR" href="#PIR"><pre>PIR</pre></a></dt>
<dd>ceases critical releasing</dd>
<dt><a id="PI[" href="#PI["><pre>PI[</pre></a></dt>
<dd>begins critical section</dd>
<dt><a id="PI]" href="#PI]"><pre>PI]</pre></a></dt>
<dd>ceases critical section</dd>
<dt><a id="PWd" href="#PWd"><pre>PWd</pre></a></dt>
<dd>begins distribute</dd>
<dt><a id="PWD" href="#PWD"><pre>PWD</pre></a></dt>
<dd>ceases distribute</dd>
<dt><a id="PWy" href="#PWy"><pre>PWy</pre></a></dt>
<dd>begins dynamic for init</dd>
<dt><a id="PWY" href="#PWY"><pre>PWY</pre></a></dt>
<dd>ceases dynamic for init</dd>
<dt><a id="PWc" href="#PWc"><pre>PWc</pre></a></dt>
<dd>begins dynamic for chunk</dd>
<dt><a id="PWC" href="#PWC"><pre>PWC</pre></a></dt>
<dd>ceases dynamic for chunk</dd>
<dt><a id="PWs" href="#PWs"><pre>PWs</pre></a></dt>
<dd>begins static for</dd>
<dt><a id="PWS" href="#PWS"><pre>PWS</pre></a></dt>
<dd>ceases static for</dd>
<dt><a id="PWe" href="#PWe"><pre>PWe</pre></a></dt>
<dd>begins section</dd>
<dt><a id="PWE" href="#PWE"><pre>PWE</pre></a></dt>
<dd>ceases section</dd>
<dt><a id="PWi" href="#PWi"><pre>PWi</pre></a></dt>
<dd>begins single</dd>
<dt><a id="PWI" href="#PWI"><pre>PWI</pre></a></dt>
<dd>ceases single</dd>
<dt><a id="PTa" href="#PTa"><pre>PTa</pre></a></dt>
<dd>begins task allocation</dd>
<dt><a id="PTA" href="#PTA"><pre>PTA</pre></a></dt>
<dd>ceases task allocation</dd>
<dt><a id="PTc" href="#PTc"><pre>PTc</pre></a></dt>
<dd>begins checking task dependencies</dd>
<dt><a id="PTC" href="#PTC"><pre>PTC</pre></a></dt>
<dd>ceases checking task dependencies</dd>
<dt><a id="PTd" href="#PTd"><pre>PTd</pre></a></dt>
<dd>begins duplicating a task</dd>
<dt><a id="PTD" href="#PTD"><pre>PTD</pre></a></dt>
<dd>ceases duplicating a task</dd>
<dt><a id="PTr" href="#PTr"><pre>PTr</pre></a></dt>
<dd>begins releasing task dependencies</dd>
<dt><a id="PTR" href="#PTR"><pre>PTR</pre></a></dt>
<dd>ceases releasing task dependencies</dd>
<dt><a id="PT[" href="#PT["><pre>PT[</pre></a></dt>
<dd>begins running a task</dd>
<dt><a id="PT]" href="#PT]"><pre>PT]</pre></a></dt>
<dd>ceases running a task</dd>
<dt><a id="PTi" href="#PTi"><pre>PTi</pre></a></dt>
<dd>begins running an if0 task</dd>
<dt><a id="PTI" href="#PTI"><pre>PTI</pre></a></dt>
<dd>ceases running an if0 task</dd>
<dt><a id="PTs" href="#PTs"><pre>PTs</pre></a></dt>
<dd>begins scheduling a task</dd>
<dt><a id="PTS" href="#PTS"><pre>PTS</pre></a></dt>
<dd>ceases scheduling a task</dd>
<dt><a id="PTg" href="#PTg"><pre>PTg</pre></a></dt>
<dd>begins a taskgroup</dd>
<dt><a id="PTG" href="#PTG"><pre>PTG</pre></a></dt>
<dd>ceases a taskgroup</dd>
<dt><a id="PTt" href="#PTt"><pre>PTt</pre></a></dt>
<dd>begins a taskwait</dd>
<dt><a id="PTT" href="#PTT"><pre>PTT</pre></a></dt>
<dd>ceases a taskwait</dd>
<dt><a id="PTw" href="#PTw"><pre>PTw</pre></a></dt>
<dd>begins waiting for taskwait dependencies</dd>
<dt><a id="PTW" href="#PTW"><pre>PTW</pre></a></dt>
<dd>ceases waiting for taskwait dependencies</dd>
<dt><a id="PTy" href="#PTy"><pre>PTy</pre></a></dt>
<dd>begins a taskyield</dd>
<dt><a id="PTY" href="#PTY"><pre>PTY</pre></a></dt>
<dd>ceases a taskyield</dd>
<dt><a id="PA[" href="#PA["><pre>PA[</pre></a></dt>
<dd>enters the attached state</dd>
<dt><a id="PA]" href="#PA]"><pre>PA]</pre></a></dt>
<dd>leaves the attached state</dd>
<dt><a id="PMi" href="#PMi"><pre>PMi</pre></a></dt>
<dd>begins microtask internal</dd>
<dt><a id="PMI" href="#PMI"><pre>PMI</pre></a></dt>
<dd>ceases microtask internal</dd>
<dt><a id="PMu" href="#PMu"><pre>PMu</pre></a></dt>
<dd>begins microtask user code</dd>
<dt><a id="PMU" href="#PMU"><pre>PMU</pre></a></dt>
<dd>ceases microtask user code</dd>
<dt><a id="PH[" href="#PH["><pre>PH[</pre></a></dt>
<dd>begins worker loop</dd>
<dt><a id="PH]" href="#PH]"><pre>PH]</pre></a></dt>
<dd>ceases worker loop</dd>
<dt><a id="PCf" href="#PCf"><pre>PCf</pre></a></dt>
<dd>begins fork call</dd>
<dt><a id="PCF" href="#PCF"><pre>PCF</pre></a></dt>
<dd>ceases fork call</dd>
<dt><a id="PCi" href="#PCi"><pre>PCi</pre></a></dt>
<dd>begins initialization</dd>
<dt><a id="PCI" href="#PCI"><pre>PCI</pre></a></dt>
<dd>ceases initialization</dd>
</dl>

## Model tampi

List of events for the model *tampi* with identifier **`T`** at version `1.0.0`:
<dl>
<dt><a id="TCi" href="#TCi"><pre>TCi</pre></a></dt>
<dd>starts issuing a non-blocking communication operation</dd>
<dt><a id="TCI" href="#TCI"><pre>TCI</pre></a></dt>
<dd>stops  issuing a non-blocking communication operation</dd>
<dt><a id="TGc" href="#TGc"><pre>TGc</pre></a></dt>
<dd>starts checking pending requests from the global array</dd>
<dt><a id="TGC" href="#TGC"><pre>TGC</pre></a></dt>
<dd>stops  checking pending requests from the global array</dd>
<dt><a id="TLi" href="#TLi"><pre>TLi</pre></a></dt>
<dd>enters the library code at an API function</dd>
<dt><a id="TLI" href="#TLI"><pre>TLI</pre></a></dt>
<dd>leaves the library code at an API function</dd>
<dt><a id="TLp" href="#TLp"><pre>TLp</pre></a></dt>
<dd>enters the library code at a polling function</dd>
<dt><a id="TLP" href="#TLP"><pre>TLP</pre></a></dt>
<dd>leaves the library code at a polling function</dd>
<dt><a id="TQa" href="#TQa"><pre>TQa</pre></a></dt>
<dd>starts adding a ticket/requests to a queue</dd>
<dt><a id="TQA" href="#TQA"><pre>TQA</pre></a></dt>
<dd>stops  adding a ticket/requests to a queue</dd>
<dt><a id="TQt" href="#TQt"><pre>TQt</pre></a></dt>
<dd>starts transferring tickets/requests from queues to global array</dd>
<dt><a id="TQT" href="#TQT"><pre>TQT</pre></a></dt>
<dd>stops  transferring tickets/requests from queues to global array</dd>
<dt><a id="TRc" href="#TRc"><pre>TRc</pre></a></dt>
<dd>starts processsing a completed request</dd>
<dt><a id="TRC" href="#TRC"><pre>TRC</pre></a></dt>
<dd>stops  processsing a completed request</dd>
<dt><a id="TRt" href="#TRt"><pre>TRt</pre></a></dt>
<dd>starts testing a single request with MPI_Test</dd>
<dt><a id="TRT" href="#TRT"><pre>TRT</pre></a></dt>
<dd>stops  testing a single request with MPI_Test</dd>
<dt><a id="TRa" href="#TRa"><pre>TRa</pre></a></dt>
<dd>starts testing several requests with MPI_Testall</dd>
<dt><a id="TRA" href="#TRA"><pre>TRA</pre></a></dt>
<dd>stops  testing several requests with MPI_Testall</dd>
<dt><a id="TRs" href="#TRs"><pre>TRs</pre></a></dt>
<dd>starts testing several requests with MPI_Testsome</dd>
<dt><a id="TRS" href="#TRS"><pre>TRS</pre></a></dt>
<dd>stops  testing several requests with MPI_Testsome</dd>
<dt><a id="TTc" href="#TTc"><pre>TTc</pre></a></dt>
<dd>starts creating a ticket linked to a set of requests and a task</dd>
<dt><a id="TTC" href="#TTC"><pre>TTC</pre></a></dt>
<dd>stops  creating a ticket linked to a set of requests and a task</dd>
<dt><a id="TTw" href="#TTw"><pre>TTw</pre></a></dt>
<dd>starts waiting for a ticket completion</dd>
<dt><a id="TTW" href="#TTW"><pre>TTW</pre></a></dt>
<dd>stops  waiting for a ticket completion</dd>
</dl>

## Model nosv

List of events for the model *nosv* with identifier **`V`** at version `2.3.0`:
<dl>
<dt><a id="VTc" href="#VTc"><pre>VTc(u32 taskid, u32 typeid)</pre></a></dt>
<dd>creates task %{taskid} with type %{typeid}</dd>
<dt><a id="VTC" href="#VTC"><pre>VTC(u32 taskid, u32 typeid)</pre></a></dt>
<dd>creates parallel task %{taskid} with type %{typeid}</dd>
<dt><a id="VTx" href="#VTx"><pre>VTx(u32 taskid, u32 bodyid)</pre></a></dt>
<dd>executes the task %{taskid} with bodyid %{bodyid}</dd>
<dt><a id="VTe" href="#VTe"><pre>VTe(u32 taskid, u32 bodyid)</pre></a></dt>
<dd>ends the task %{taskid} with bodyid %{bodyid}</dd>
<dt><a id="VTp" href="#VTp"><pre>VTp(u32 taskid, u32 bodyid)</pre></a></dt>
<dd>pauses the task %{taskid} with bodyid %{bodyid}</dd>
<dt><a id="VTr" href="#VTr"><pre>VTr(u32 taskid, u32 bodyid)</pre></a></dt>
<dd>resumes the task %{taskid} with bodyid %{bodyid}</dd>
<dt><a id="VYc" href="#VYc"><pre>VYc+(u32 typeid, str label)</pre></a></dt>
<dd>creates task type %{typeid} with label &quot;%{label}&quot;</dd>
<dt><a id="VSr" href="#VSr"><pre>VSr</pre></a></dt>
<dd>receives a task from another thread</dd>
<dt><a id="VSs" href="#VSs"><pre>VSs</pre></a></dt>
<dd>sends a task to another thread</dd>
<dt><a id="VS@" href="#VS@"><pre>VS@</pre></a></dt>
<dd>self assigns itself a task</dd>
<dt><a id="VSh" href="#VSh"><pre>VSh</pre></a></dt>
<dd>enters the hungry state, waiting for work</dd>
<dt><a id="VSf" href="#VSf"><pre>VSf</pre></a></dt>
<dd>is no longer hungry</dd>
<dt><a id="VS[" href="#VS["><pre>VS[</pre></a></dt>
<dd>enters scheduler server mode</dd>
<dt><a id="VS]" href="#VS]"><pre>VS]</pre></a></dt>
<dd>leaves scheduler server mode</dd>
<dt><a id="VU[" href="#VU["><pre>VU[</pre></a></dt>
<dd>starts submitting a task</dd>
<dt><a id="VU]" href="#VU]"><pre>VU]</pre></a></dt>
<dd>stops  submitting a task</dd>
<dt><a id="VMa" href="#VMa"><pre>VMa</pre></a></dt>
<dd>starts allocating memory</dd>
<dt><a id="VMA" href="#VMA"><pre>VMA</pre></a></dt>
<dd>stops  allocating memory</dd>
<dt><a id="VMf" href="#VMf"><pre>VMf</pre></a></dt>
<dd>starts freeing memory</dd>
<dt><a id="VMF" href="#VMF"><pre>VMF</pre></a></dt>
<dd>stops  freeing memory</dd>
<dt><a id="VAr" href="#VAr"><pre>VAr</pre></a></dt>
<dd>enters nosv_create()</dd>
<dt><a id="VAR" href="#VAR"><pre>VAR</pre></a></dt>
<dd>leaves nosv_create()</dd>
<dt><a id="VAd" href="#VAd"><pre>VAd</pre></a></dt>
<dd>enters nosv_destroy()</dd>
<dt><a id="VAD" href="#VAD"><pre>VAD</pre></a></dt>
<dd>leaves nosv_destroy()</dd>
<dt><a id="VAs" href="#VAs"><pre>VAs</pre></a></dt>
<dd>enters nosv_submit()</dd>
<dt><a id="VAS" href="#VAS"><pre>VAS</pre></a></dt>
<dd>leaves nosv_submit()</dd>
<dt><a id="VAp" href="#VAp"><pre>VAp</pre></a></dt>
<dd>enters nosv_pause()</dd>
<dt><a id="VAP" href="#VAP"><pre>VAP</pre></a></dt>
<dd>leaves nosv_pause()</dd>
<dt><a id="VAy" href="#VAy"><pre>VAy</pre></a></dt>
<dd>enters nosv_yield()</dd>
<dt><a id="VAY" href="#VAY"><pre>VAY</pre></a></dt>
<dd>leaves nosv_yield()</dd>
<dt><a id="VAw" href="#VAw"><pre>VAw</pre></a></dt>
<dd>enters nosv_waitfor()</dd>
<dt><a id="VAW" href="#VAW"><pre>VAW</pre></a></dt>
<dd>leaves nosv_waitfor()</dd>
<dt><a id="VAc" href="#VAc"><pre>VAc</pre></a></dt>
<dd>enters nosv_schedpoint()</dd>
<dt><a id="VAC" href="#VAC"><pre>VAC</pre></a></dt>
<dd>leaves nosv_schedpoint()</dd>
<dt><a id="VAa" href="#VAa"><pre>VAa</pre></a></dt>
<dd>enters nosv_attach()</dd>
<dt><a id="VAA" href="#VAA"><pre>VAA</pre></a></dt>
<dd>leaves nosv_attach()</dd>
<dt><a id="VAe" href="#VAe"><pre>VAe</pre></a></dt>
<dd>enters nosv_detach()</dd>
<dt><a id="VAE" href="#VAE"><pre>VAE</pre></a></dt>
<dd>leaves nosv_detach()</dd>
<dt><a id="VAl" href="#VAl"><pre>VAl</pre></a></dt>
<dd>enters nosv_mutex_lock()</dd>
<dt><a id="VAL" href="#VAL"><pre>VAL</pre></a></dt>
<dd>leaves nosv_mutex_lock()</dd>
<dt><a id="VAt" href="#VAt"><pre>VAt</pre></a></dt>
<dd>enters nosv_mutex_trylock()</dd>
<dt><a id="VAT" href="#VAT"><pre>VAT</pre></a></dt>
<dd>leaves nosv_mutex_trylock()</dd>
<dt><a id="VAu" href="#VAu"><pre>VAu</pre></a></dt>
<dd>enters nosv_mutex_unlock()</dd>
<dt><a id="VAU" href="#VAU"><pre>VAU</pre></a></dt>
<dd>leaves nosv_mutex_unlock()</dd>
<dt><a id="VAb" href="#VAb"><pre>VAb</pre></a></dt>
<dd>enters nosv_barrier_wait()</dd>
<dt><a id="VAB" href="#VAB"><pre>VAB</pre></a></dt>
<dd>leaves nosv_barrier_wait()</dd>
<dt><a id="VHa" href="#VHa"><pre>VHa</pre></a></dt>
<dd>enters nosv_attach()</dd>
<dt><a id="VHA" href="#VHA"><pre>VHA</pre></a></dt>
<dd>leaves nosv_dettach()</dd>
<dt><a id="VHw" href="#VHw"><pre>VHw</pre></a></dt>
<dd>begins execution as worker</dd>
<dt><a id="VHW" href="#VHW"><pre>VHW</pre></a></dt>
<dd>ceases execution as worker</dd>
<dt><a id="VHd" href="#VHd"><pre>VHd</pre></a></dt>
<dd>begins execution as delegate</dd>
<dt><a id="VHD" href="#VHD"><pre>VHD</pre></a></dt>
<dd>ceases execution as delegate</dd>
<dt><a id="VPp" href="#VPp"><pre>VPp</pre></a></dt>
<dd>sets progress state to Progressing</dd>
<dt><a id="VPr" href="#VPr"><pre>VPr</pre></a></dt>
<dd>sets progress state to Resting</dd>
<dt><a id="VPa" href="#VPa"><pre>VPa</pre></a></dt>
<dd>sets progress state to Absorbing</dd>
</dl>
