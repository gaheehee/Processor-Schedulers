## Project #2: Simulating Processor Schedulers

### Goal

We have learned various process scheduling policies and examined their properties in the class.
To better understand them, you will implement SJF, SRTF, round-robin, priority, priority + aging, priority + PIP, and priority + PCP scheduling policies on an educational scheduler simulation framework which imitates the scheduler core of modern operating systems.


### description how each scheduling policy is implemented
	
- **Sjf_scheduling**: sjf는 no preemption이므로 만약 현재 돌고 있는 current process가 있다면, 계
속 돌게끔 current를 리턴해주고, 현재 돌고 있는 process가 없다면, list_head의 순회 함수를
이용하여 ready하고 있는 process들 중에 가장 작은 lifespan을 가지고 있는 process를 찾아서
리턴하도록 구현하였다. 가장 작은 lifespan을 가지고 있는 process가 여럿이면 그 중에 상대
적으로 먼저 기다린 process를 리턴해주었다.
	
- **Srtf_scheduling**: srtf는 preemption이 가능하므로 현재 process가 돌고 있다면, 그 process를
ready 상태로 바꾼 뒤 readyqueue에 넣어주고, readyqueue에서 list_head의 순회 함수를 이용
하여 ready하고 있는 process들 중에 remain(lifespan – age)가 작은 process를 선택하여
return해주도록 구현하였다. 가장 작은 remain을 가지고 있는 process가 여럿이면 그 중에 상
대적으로 먼저 기다린 process를 리턴해주었다.
	
- **rr_scheduling**: rr도 preemption이 가능하고 한 tick동안 각각 다른 process들이 번갈아가며 실
행하기 때문에, 현재 process가 돌고 있다면, 그 process를 readyqueue에 넣어주고
readyqueue에서 기다리고 있는 다음 process를 선택하여 리턴하도록 구현해줬다.
	
- 위의 policy(sjf, srtf, rr)들은 resource를 잡으려고 할 때 resource의 주인이 없다면 resource를
잡고, 주인이 있다면 resource의 waitqueue에 들어가 있도록 구현해주었다. 그리고 resouce를
release할 땐, waitqueue에 온 순서대로 resource를 넘겨주도록 구현하였다. (fcfs의 acquire, 
release 사용)
	
- **Prio_scheduling**: preemption이 가능하므로 현재 돌고 있는 process가 있다면 readyqueue에
넣어주었고, 다음 실행시킬 process는 list_head 순회함수를 이용하여 readyqueue에서 ready 
중인 process들 중에 priority가 가장 높은 process를 선택하여 리턴하여줬다. 만약에 가장 높
은 priority를 동일하게 가지고 있는 process가 여러 개 존재한다면 rr처럼 번갈아가며 실행할
수 있도록 하였다. 이는 current process를 readyqueue에 넣을 때, 가장 끝(tail)에 넣고 다음
가장 높은 process를 뽑을 때는 readyqueue에서 priority가 가장 큰 process들 중에 앞쪽에
있는 proess를 뽑도록 하여 같은 가장 큰 priority를 가진 process가 있다면 번갈아가며 rr처럼
실행되도록 하였다.
	
- **Pa_scheduling**: preemption이 가능하므로 현재 process가 돌고있다면 readyqueue에 넣어주고,
다음 실행시킬 process는 prio와 동일하게 list_head 순회함수를 이용하여 priority가 가장 높
은 process로 뽑아주고, 나머지 readyqueue에 존재하는 process들은 priority를 +1씩 해준다.
그리고 다음에 실행시킬 process로 뽑힌 process는 원래 자신의 priority로 되돌려주었다. 그리
고 prio_scheduling과 동일하게 current process를 readyqueue에 넣을 때, 가장 끝(tail)에 넣고
다음 가장 높은 process를 뽑을 때는 readyqueue에서 priority가 가장 큰 process들 중에 앞
쪽에 있는 proess를 뽑도록 하여 같은 가장 큰 priority를 가진 process가 있다면 번갈아가며
rr처럼 실행되도록 하였다.
	
- Prio와 pa scheduling policy에선 resource를 잡으려고 할 때 fcfs와 동일하게 resource의 주인
이 없다면 resource를 잡고, 주인이 있다면 resource의 waitqueue에 들어가 있도록 구현해주
었다. 그리고 resource를 release할 때는 release하고 나서, resource의 waitqueue에 있는
process들 중에서 가장 높은 priority를 가지고 있는 process에게 resource를 넘겨주었다. 그리
고 가장 큰 priority를 가진 process가 여러 개라면 waitqueue에서 rr처럼 번갈아가면서 선택
하도록 상대적으로 먼저 기다린 process를 선택한 후에 waitqueue 가장 뒤쪽에 넣어주어 구
현해주었다. (fcfs의 acquire 사용, prio_release 구현)
	
- **Pcp_scheduling**: Pcp_scheduler는 prio_scheduler와 동일하게 구현해주었다. Pcp_acquair에서는
resource를 쓰는 process가 없다면, resource의 주인을 자기로 한 후에 자신의 priority를
MAX_PRIO로 바꾸어 준다. 만약에 쓰는 애가 있다면, resource의 wait queue에 들어가서 기다
린다. Pcp_release에서는 원래 자기자신의 priority로 되돌아간 다음에 resource 소유를 취소한
다. 그리고 list_head 순회함수를 이용하여 resource wait queue에서 가장 큰 priority를 갖는
process를 선택하여 resource wait queue에서 삭제한 후 그 process의 priority를 MAX_PRIO로
바꾸어 준 다음 ready queue 안에 넣어서 다음 resource 잡을 애를 처리해주었다.
	
- **Pip_scheduling**: pip_scheduler는 prio_scheduler와 동일하게 구현해 주었다. 그러나
pip_acquire과 pip_release는 따로 구현해주었다. Pip_acquire에서는 resource를 쓰고 있는
process가 없다면, resource의 주인을 자기로 한 후에 list_head 순회함수를 이용하여 만약
resource wait queue에 들어있는 process들이 가진 priority 중 최대 priority가 자신보다 크다
면 최대 priority를 상속받는다. resource를 사용하고 있는 process가 있다면, wait 하기 전에
resource를 쥐고 있는 process의 priority가 자신의 것보다 작으면 자신의 priority를 resource
를 쥐고 있는 process에게 상속해주고 resource의 wait queue에 들어간다. Pip_release에서는
원래 자신의 priority로 돌아간 다음에 resource를 방출하였다. 그 후 resource의 wait queue에
서 prio_origin이 가장 큰 process를 다음에 resource를 쥘 process로 선택하였다. Resource 
wait queue에서 prio_origin이 얘보다 큰 것이 있다면 상속시켜 준 후, ready queue에 넣어주
었다.
