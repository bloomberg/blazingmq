------------------------- MODULE BlazingMQLeaderElection -------------------------
EXTENDS Naturals, FiniteSets, Sequences, Reals, TLC

\* Input parameters
CONSTANTS   Server, \** The servers involved. E.g. {S1, S2, S3}
            MaxRestarts, \** Maximum number of times a server should restart, to limit the state space.
            MaxScouting, \** Maximum number of times a server should send a ScoutingRequest message, to limit the state space.
            MaxUnavailable \** Maximum number of times a server should be

\* Model values
CONSTANTS Follower, Candidate, Leader
CONSTANTS Nil

CONSTANTS ElectionProposal, ElectionResponse,
          LeaderHeartbeat, HeartbeatResponse,
          ScoutingRequest, ScoutingResponse,
          LeadershipCession, NodeUnavailable


----
\* Global variables
VARIABLE messages

----
\* Auxilliary variables (used for state-space control)
VARIABLE restartCounter, scoutingCounter, unavailableCounter
auxVars == <<restartCounter, scoutingCounter, unavailableCounter>>

----
\* Per server variables
VARIABLE currentTerm

VARIABLE state

VARIABLE leaderId

VARIABLE tentativeLeaderId

VARIABLE supporters

VARIABLE scoutingInfo

serverVars == <<currentTerm, state, leaderId, tentativeLeaderId, supporters, scoutingInfo>>

allVars == <<messages, serverVars, auxVars>>
symmServers == Permutations(Server)

InitServerVars == /\ currentTerm = [i \in Server |-> 0]
                  /\ state = [i \in Server |-> Follower]
                  /\ leaderId = [i \in Server |-> Nil]
                  /\ tentativeLeaderId = [i \in Server |-> Nil]
                  /\ supporters = [i \in Server |-> {}]
                  /\ scoutingInfo = [i \in Server |-> [term |-> Nil, responses |-> {}]]

InitAuxVars == /\ restartCounter = [i \in Server |-> 0]
               /\ scoutingCounter = [i \in Server |-> 0]
               /\ unavailableCounter = [i \in Server |-> 0]

Init == /\ messages = [m \in {} |-> 0]
        /\ InitServerVars
        /\ InitAuxVars

----
\* Helpers

\* The number of servers required to achieve quorum
Quorum == Cardinality(Server) \div 2  + 1

ResetScoutingInfo == [term |-> Nil, responses |-> {}]

\* Add a response from node j to the scouting info, which indicates support
WithResponse(scoutingInfoP, j) ==
   [term |-> scoutingInfoP.term, responses |-> scoutingInfoP.responses \union  {j}]

\* Remove support from node j from the scouting info
WithoutResponse(scoutingInfoP, j) ==
   [term |-> scoutingInfoP.term, responses |-> scoutingInfoP.responses \ {j}]


----
\* Message sending helpers

\* Will only send the messages if it hasn't done so before
SendMultipleOnce(msgs) ==
    /\ \A m \in msgs : m \notin DOMAIN messages
    /\ messages' = messages @@ [msg \in msgs |-> 1]


\* Note: a message can only match an existing message if it is
\* identical (all fields).
Send(m) ==
    IF m \in DOMAIN messages
    THEN messages' = [messages EXCEPT ![m] = @ + 1]
    ELSE messages' = messages @@ (m :> 1)

\* Explicit duplicate operator for when we purposefully want message duplication
Duplicate(m) ==
    /\ m \in DOMAIN messages
    /\ messages' = [messages EXCEPT ![m] = @ + 1]

\* Remove a message from the bag of messages. Used when a server is done
\* processing a message.
Discard(m) ==
    /\ m \in DOMAIN messages
    /\ messages[m] > 0 \* message must exist
    /\ messages' = [messages EXCEPT ![m] = @ - 1]


\* Discard the request and add the response to the bag of messages
Reply(response, request) ==
    /\ messages[request] > 0 \* request must exist
    /\ IF response \notin DOMAIN messages \* response does not exist, so add it
       THEN messages' = [messages EXCEPT ![request] = @ - 1] @@ (response :> 1)
       ELSE messages' = [messages EXCEPT ![request] = @ - 1,
                                         ![response] = @ + 1] \* response was sent previously, so increment delivery counter

----
\* Define server behaviours

\* Server i restarts from stable storage.
\* It resets every server variable but its currentTerm
Restart(i) ==
    /\ restartCounter[i] < MaxRestarts
    /\ state' = [state EXCEPT ![i] = Follower]
    /\ leaderId' = [leaderId EXCEPT ![i] = Nil]
    /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = Nil]
    /\ supporters' = [supporters EXCEPT ![i] = {}]
    /\ scoutingInfo' = [scoutingInfo EXCEPT ![i] = ResetScoutingInfo]
    /\ restartCounter' = [restartCounter EXCEPT ![i] = @ + 1]
    /\ UNCHANGED <<messages, currentTerm, scoutingCounter, unavailableCounter>>

\* Server i hasn't received a heartbeat event
HeartbeatTimeout(i) ==
    /\ state[i] = Follower
    /\ Cardinality(supporters[i]) = 0
    /\ \/ /\ leaderId[i] /= Nil
          /\ tentativeLeaderId[i] = Nil
          /\ leaderId' = [leaderId EXCEPT  ![i] = Nil]
          /\ UNCHANGED <<tentativeLeaderId>>
       \/ /\ tentativeLeaderId[i] /= Nil
          /\ leaderId[i] = Nil
          /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = Nil]
          /\ UNCHANGED <<leaderId>>
    /\ UNCHANGED <<messages, currentTerm, supporters, state, scoutingInfo, auxVars>>


\* Server i hasn't received a heartbeat from the leader and
\* sends a scouting request to other nodes
StartScouting(i) ==
    /\ scoutingCounter[i] < MaxScouting
    /\ state[i] = Follower
    /\ leaderId[i] = Nil
    /\ tentativeLeaderId[i] = Nil
    /\ scoutingInfo[i] = ResetScoutingInfo
    /\ scoutingInfo' = [scoutingInfo EXCEPT ![i] = [term |-> currentTerm[i] + 1, responses |-> {i}]]
    /\ scoutingCounter' = [scoutingCounter EXCEPT ![i] = @ + 1]
    /\ SendMultipleOnce({[mtype |-> ScoutingRequest,
             mterm |-> currentTerm[i] + 1,
             msource |-> i,
             mdest |-> j] : j \in Server \ {i}})
    /\ UNCHANGED  <<currentTerm, state, supporters, leaderId, tentativeLeaderId, restartCounter, unavailableCounter>>


\* Server i has sent a scouting request but has not received enough responses
ScoutingTimeout(i) ==
    /\ state[i] = Follower
    /\ scoutingInfo[i].term /= Nil
    /\ Cardinality(scoutingInfo[i].responses) < Quorum
    /\ scoutingInfo' = [scoutingInfo EXCEPT ![i] = ResetScoutingInfo]
    /\ UNCHANGED <<currentTerm, state, supporters, leaderId, tentativeLeaderId, messages, auxVars>>


\* Server i has received majority support from its scouting request and starts a new election
RequestVote(i, j) ==
    /\ i /= j
    /\ state[i] = Follower
    /\ leaderId[i] = Nil
    /\ tentativeLeaderId[i] = Nil
    /\ scoutingInfo[i].term /= Nil
    /\ Cardinality(scoutingInfo[i].responses) >= Quorum
    /\ supporters' = [supporters EXCEPT ![i] = {i}]
    /\ currentTerm' = [currentTerm EXCEPT ![i] = scoutingInfo[i].term]
    /\ state' = [state EXCEPT ![i] = Candidate]
    /\ Send([mtype |-> ElectionProposal,
             mterm |-> scoutingInfo[i].term,
             msource |-> i,
             mdest |-> j])
    /\ scoutingInfo' = [scoutingInfo EXCEPT ![i] = ResetScoutingInfo]
    /\ UNCHANGED <<leaderId, tentativeLeaderId, auxVars>>

\* Server i proposed an election but did not receive enough supporters in a period, transition to follower
ElectionTimeout(i) ==
    /\ state[i] = Candidate
    /\ leaderId[i] = Nil
    /\ tentativeLeaderId[i] = Nil
    /\ Cardinality(supporters[i]) < Quorum
    /\ state' = [state EXCEPT ![i] = Follower]
    /\ supporters' = [supporters EXCEPT ![i] = {}]
    /\ UNCHANGED <<leaderId, tentativeLeaderId, scoutingInfo, messages, currentTerm, auxVars>>


\* Server i proposed an election and has received support from a majority of nodes
BecomeLeader(i) ==
    /\ state[i] = Candidate
    /\ leaderId[i] = Nil
    /\ tentativeLeaderId[i] = Nil
    /\ Cardinality(supporters[i]) >= Quorum
    /\ leaderId' = [leaderId EXCEPT ![i] = i]
    /\ state' = [state EXCEPT ![i] = Leader]
    /\ UNCHANGED << tentativeLeaderId, supporters, scoutingInfo, currentTerm, auxVars, messages>>

\* Server i wants to send a heartbeat message to all servers
SendHeartbeat(i) ==
    /\ state[i] = Leader
    /\ leaderId[i] = i
    /\ tentativeLeaderId[i] = Nil
    /\ supporters[i] /= {}
    /\ currentTerm[i] > 0
    /\ SendMultipleOnce({[mtype |-> LeaderHeartbeat,
             mterm |-> currentTerm[i],
             msource |-> i,
             mdest |-> j] : j \in Server \ {i}})
    /\ UNCHANGED <<serverVars, auxVars>>

MakeUnavailable(i) ==
    /\ unavailableCounter[i] < MaxUnavailable
    /\ SendMultipleOnce({[mtype |-> NodeUnavailable,
             msource |-> i,
             mdest |-> j] : j \in Server \ {i}})
    /\ unavailableCounter' = [unavailableCounter EXCEPT ![i] = @ + 1]
    /\ UNCHANGED <<serverVars, restartCounter, scoutingCounter>>

ReceivableMessage(m, mtype) ==
    /\ messages[m] > 0
    /\ m.mtype = mtype

----
\* Message handlers

\* Server i receives a request for a vote from server j. An ElectionResponse indicates support
ApplyElectionProposalEvent ==
    \E m \in DOMAIN messages :
       /\   ReceivableMessage(m, ElectionProposal)
       /\   LET i == m.mdest
                j == m.msource
            IN
                \/  /\ state[i] = Follower
                    /\ leaderId[i] = Nil \* Sticky leader. If server sees a healthy leader, don't respond to the election proposal.
                    /\ m.mterm > currentTerm[i]
                    /\ currentTerm' = [currentTerm EXCEPT ![i] = m.mterm]
                    /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = j]
                    /\ scoutingInfo' = [scoutingInfo EXCEPT ![i] = ResetScoutingInfo]
                    /\ Reply(
                       [mtype |-> ElectionResponse,
                        mterm |-> m.mterm,
                        msource |-> i,
                        mdest |-> j], m)
                    /\ UNCHANGED <<supporters, state, leaderId, auxVars>>
                \/  /\ state[i] = Candidate
                    /\ m.mterm > currentTerm[i]
                    /\ state' = [state EXCEPT  ![i] = Follower]
                    /\ currentTerm' = [currentTerm EXCEPT ![i] = m.mterm]
                    /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = j]
                    /\ leaderId' = [leaderId EXCEPT ![i] = Nil]
                    /\ supporters' = [supporters EXCEPT ![i] = {}]
                    /\ Reply(
                       [mtype |-> ElectionResponse,
                        mterm |-> m.mterm,
                        msource |-> i,
                        mdest |-> j], m)
                    /\ UNCHANGED <<scoutingInfo, auxVars>>

\* Server i receives an ElectionResponse from server j, indicating support for the ElectionProposal.
ApplyElectionResponseEvent ==
    \E m \in DOMAIN messages :
       /\    ReceivableMessage(m, ElectionResponse)
       /\   LET i == m.mdest
                j == m.msource
            IN
                /\ \/ /\ state[i] = Candidate
                      /\ m.mterm >= currentTerm[i]
                      /\ j \notin supporters[i]
                      /\ supporters' = [supporters EXCEPT ![i] = supporters[i] \union {j}]
                      /\ Discard(m)
                      /\ UNCHANGED <<state, tentativeLeaderId, leaderId, currentTerm>>
                   \/ /\ state[i] = Leader
                      /\ m.mterm >= currentTerm[i]
                      /\ IF m.mterm = currentTerm[i]
                         THEN
                           /\ supporters' = [supporters EXCEPT ![i] = supporters[i] \union {j}]
                           /\ Discard(m)
                           /\ UNCHANGED <<currentTerm, state, leaderId, tentativeLeaderId>>
                         ELSE \* Transition to follower
                           /\ state' = [state EXCEPT ![i] = Follower]
                           /\ currentTerm' = [currentTerm EXCEPT ![i] = m.mterm]
                           /\ supporters' = [supporters EXCEPT ![i] = {}]
                           /\ leaderId' = [leaderId EXCEPT ![i] = Nil]
                           /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = Nil]
                           /\ Reply(
                              [mtype |-> LeadershipCession,
                               mterm |-> m.mterm,
                               msource |-> i,
                               mdest |-> j], m)
                /\ UNCHANGED <<scoutingInfo, auxVars>>


\* Server i receives a scouting request from server j. A ScoutingResponse indicates support
ApplyScoutingRequestEvent ==
    \E m \in DOMAIN messages :
       /\ ReceivableMessage(m, ScoutingRequest)
       /\   LET i == m.mdest
                j == m.msource
            IN
               /\ i /= j
               /\ leaderId[i] = Nil
               /\ m.mterm > currentTerm[i]
               /\ Reply([mtype |-> ScoutingResponse,
                         mterm |-> currentTerm[i],
                         msource |-> i,
                         mdest |-> j], m)
               /\ UNCHANGED <<serverVars, auxVars>>


\* Server i receives a scouting response from server j, which indicates support
ApplyScoutingResponseEvent ==
    \E m \in DOMAIN messages :
       /\ ReceivableMessage(m, ScoutingResponse)
       /\   LET i == m.mdest
                j == m.msource
            IN
               /\ state[i] = Follower
               /\ leaderId[i] = Nil
               /\ tentativeLeaderId[i] = Nil
               /\ scoutingInfo[i].term /= Nil
               /\ scoutingInfo' = [scoutingInfo EXCEPT  ![i] = WithResponse(scoutingInfo[i], j)]
               /\ Discard(m)
               /\ UNCHANGED <<state, leaderId, tentativeLeaderId, supporters, currentTerm, auxVars>>


\* Server i receives a heartbeat message from server j
ApplyLeaderHeartbeatEvent ==
    \E m \in DOMAIN messages :
       /\   ReceivableMessage(m, LeaderHeartbeat)
       /\   LET i == m.mdest
                j == m.msource
            IN
                \/ /\ state[i] = Follower
                   /\  \/ currentTerm[i] = m.mterm
                           /\   \/  /\ tentativeLeaderId[i] = j
                                    /\ leaderId' = [leaderId EXCEPT ![i] = j]
                                    /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = Nil]
                                    /\ scoutingInfo' = [scoutingInfo EXCEPT ![i] = ResetScoutingInfo]
                                    /\ Discard(m)
                                    /\ UNCHANGED <<state, supporters, currentTerm, auxVars>>
                                \/  /\ tentativeLeaderId[i] = Nil
                                    /\ leaderId[i] = Nil
                                    /\ leaderId' = [leaderId EXCEPT ![i] = j]
                                    /\ scoutingInfo' = [scoutingInfo EXCEPT ![i] = ResetScoutingInfo]
                                    /\ Reply([mtype |-> ElectionResponse,
                                            mterm |-> m.mterm,
                                            msource |-> i,
                                            mdest |-> j], m)
                                    /\ UNCHANGED <<state, supporters, currentTerm, tentativeLeaderId, auxVars>>
                       \/ currentTerm[i] > m.mterm
                              /\ Reply([mtype |-> HeartbeatResponse,
                                       mterm |-> currentTerm[i],
                                       msource |-> i,
                                       mdest |-> j], m)
                              /\ UNCHANGED <<serverVars, auxVars>>
                       \/ currentTerm[i] < m.mterm
                              /\ leaderId' = [leaderId EXCEPT  ![i] = j]
                              /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = Nil]
                              /\ currentTerm' = [currentTerm EXCEPT ![i] = m.mterm]
                              /\ scoutingInfo' = [scoutingInfo EXCEPT ![i] = ResetScoutingInfo]
                              /\ Reply([mtype |-> ElectionResponse,
                                       mterm |-> m.mterm,
                                       msource |-> i,
                                       mdest |-> j], m)
                              /\ UNCHANGED <<state, supporters, auxVars>>
                \/ /\ state[i] = Candidate
                   /\ IF currentTerm[i] > m.mterm
                      THEN
                         /\ Reply([mtype |-> HeartbeatResponse,
                                  mterm |-> currentTerm[i],
                                  msource |-> i,
                                  mdest |-> j], m)
                         /\ UNCHANGED <<serverVars, auxVars>>
                      ELSE
                         /\ supporters' = [supporters EXCEPT ![i] = {}]
                         /\ state' = [state EXCEPT ![i] = Follower]
                         /\ currentTerm' = [currentTerm EXCEPT ![i] = m.mterm]
                         /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = Nil]
                         /\ leaderId' = [leaderId EXCEPT ![i] = j]
                         /\ Reply([mtype |-> ElectionResponse,
                                  mterm |-> m.mterm,
                                  msource |-> i,
                                  mdest |-> j], m)
                         /\ UNCHANGED <<scoutingInfo, auxVars>>
                \/ /\ state[i] = Leader
                   /\ IF currentTerm[i] >= m.mterm
                      THEN
                         /\ Reply([mtype |-> LeaderHeartbeat,
                                  mterm |-> currentTerm[i],
                                  msource |-> i,
                                  mdest |-> j], m)
                         /\ UNCHANGED <<serverVars, auxVars>>
                      ELSE
                         /\ supporters' = [supporters EXCEPT ![i] = {}]
                         /\ state' = [state EXCEPT  ![i] = Follower]
                         /\ currentTerm' = [currentTerm EXCEPT ![i] = m.mterm]
                         /\ leaderId' = [leaderId EXCEPT ![i] = j]
                         /\ Reply([mtype |-> LeadershipCession,
                                  mterm |-> m.mterm,
                                  msource |-> i,
                                  mdest |-> j], m)
                         /\ UNCHANGED <<scoutingInfo, tentativeLeaderId, auxVars>>



\* Server i receives a heartbeat response from server j
ApplyHeartbeatResponseEvent  ==
    \E m \in DOMAIN messages :
       /\ ReceivableMessage(m, HeartbeatResponse)
       /\   LET i == m.mdest
                j == m.msource
            IN
               /\ state[i] = Leader
               /\ currentTerm[i] < m.mterm \* Do nothing otherwise
               /\ state' = [state EXCEPT ![i] = Follower]
               /\ currentTerm' = [currentTerm EXCEPT ![i] = m.mterm]
               /\ leaderId' = [leaderId EXCEPT ![i] = Nil]
               /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = Nil]
               /\ supporters' = [supporters EXCEPT ![i] = {}]
               /\ Reply([mtype |-> LeadershipCession,
                        mterm |-> m.mterm,
                        msource |-> i,
                        mdest |-> j], m)
               /\ UNCHANGED <<scoutingInfo, auxVars>>


\* Server i receives leadership cession notification from server j
ApplyLeadershipCessionEvent ==
    \E m \in DOMAIN messages :
       /\ ReceivableMessage(m, LeadershipCession)
       /\   LET i == m.mdest
                j == m.msource
            IN
               /\ state[i] = Follower
               /\ Cardinality(supporters[i]) = 0
               /\ currentTerm[i] <= m.mterm
               /\ leaderId' = [leaderId EXCEPT ![i] = IF leaderId[i] = j THEN Nil ELSE leaderId[i]]
               /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = IF tentativeLeaderId[i] = j THEN Nil ELSE tentativeLeaderId[i]]
               /\ Discard(m)
               /\ UNCHANGED <<state, currentTerm, scoutingInfo, supporters, auxVars>>

\* Server i receives a message from server j, indicating that j is unavailable
ApplyAvailability ==
    \E m \in DOMAIN messages:
       /\ ReceivableMessage(m, NodeUnavailable)
       /\ LET i == m.mdest
              j == m.msource
          IN
            /\  Discard(m)
            /\ \/ /\ state[i] = Follower
                  /\ \/ /\ leaderId[i] = j
                        /\ tentativeLeaderId[i] = Nil
                        /\ supporters[i] = {}
                        /\ leaderId' = [leaderId EXCEPT ![i] = Nil]
                        /\ UNCHANGED <<tentativeLeaderId, scoutingInfo>>
                     \/ /\ tentativeLeaderId[i] = j
                        /\ leaderId[i] = Nil
                        /\ supporters[i] = {}
                        /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = Nil]
                        /\ UNCHANGED <<leaderId, scoutingInfo>>
                     \/ /\ scoutingInfo' = [scoutingInfo EXCEPT ![i] = WithoutResponse(scoutingInfo[i], j)]
                        /\ UNCHANGED <<tentativeLeaderId, leaderId>>
                  /\ UNCHANGED <<auxVars, state, supporters, currentTerm>>
               \/ /\ state[i] = Candidate
                  /\ supporters' = [supporters EXCEPT ![i] = supporters[i] \ {j}]
                  /\ UNCHANGED <<auxVars, state, currentTerm, tentativeLeaderId, scoutingInfo, leaderId>>
               \/ /\ state[i] = Leader
                  /\ IF Cardinality(supporters[i] \ {j}) >= Quorum
                     THEN /\ supporters' = [supporters EXCEPT ![i] = supporters[i] \ {j}]
                          /\ UNCHANGED <<auxVars, state, currentTerm, tentativeLeaderId, leaderId, scoutingInfo>>
                     ELSE
                          /\ state' = [state EXCEPT ![i] = Follower]
                          /\ tentativeLeaderId' = [tentativeLeaderId EXCEPT ![i] = Nil]
                          /\ leaderId' = [leaderId EXCEPT ![i] = Nil]
                          /\ supporters' = [supporters EXCEPT  ![i] = {}]
                          /\ SendMultipleOnce({[mtype |-> LeadershipCession,
                                                mterm |-> currentTerm[i],
                                                msource |-> i,
                                                mdest |-> k] : k \in Server \ {i}})
                          /\ UNCHANGED <<auxVars, currentTerm, scoutingInfo>>


ApplyEvent ==
        \/ ApplyElectionProposalEvent
        \/ ApplyElectionResponseEvent
        \/ ApplyScoutingRequestEvent
        \/ ApplyScoutingResponseEvent
        \/ ApplyLeadershipCessionEvent
        \/ ApplyLeaderHeartbeatEvent
        \/ ApplyHeartbeatResponseEvent
        \/ ApplyAvailability

\* End of message handlers.
----
\* Network state transitions

\* The network duplicates a message
\* There is no state-space control for this action, it causes
\* infinite state space
DuplicateMessage(m) ==
    /\ Duplicate(m)
    /\ UNCHANGED <<serverVars, auxVars>>

\* The network drops a message
\* In reality is not required as the specification
\* does not force any server to receive a message, so we
\* already get this for free.
DropMessage(m) ==
    /\ Discard(m)
    /\ UNCHANGED <<serverVars, auxVars>>

----
\* Defines how the variables may transition
Next == \/ \E i \in Server : Restart(i)
        \/ \E i \in Server : HeartbeatTimeout(i)
        \/ \E i \in Server : ScoutingTimeout(i)
        \/ \E i \in Server : ElectionTimeout(i)
        \/ \E i \in Server : BecomeLeader(i)
        \/ \E i \in Server : SendHeartbeat(i)
        \/ \E i \in Server : MakeUnavailable(i)
        \/ \E i \in Server : StartScouting(i)
        \/ \E i, j \in Server : RequestVote(i, j)
        \/ ApplyEvent
\*      \/ \E m \in DOMAIN messages : DuplicateMessage(m)
\*      \/ \E m \in DOMAIN messages : DropMessage(m)


\* The specification must start with the initial state and transition according
\* to Next.
Spec == Init /\ [][Next]_allVars

\* Invariants
BothLeader( i, j ) ==
    /\ i /= j
    /\ currentTerm[i] = currentTerm[j]
    /\ state[i] = Leader
    /\ state[j] = Leader

MoreThanOneLeader ==
    \E i, j \in Server :  BothLeader( i, j )

NotMoreThanOneLeader == \lnot MoreThanOneLeader

\* State Constraints
TermConstraint == \A i \in Server: currentTerm[i] < 3

=============================================================================
\* Modification History
\* Last modified Fri May 12 15:46:57 BST 2023 by Timi Adeniran
\* Created Wed Jun 29 15:06:13 BST 2022 by Timi Adeniran
