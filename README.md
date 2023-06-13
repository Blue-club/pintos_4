# 💡Pintos_4팀 WIL - Project 1.
## 일별 진행 목록 📆 

### 23.05.26(금)
- github team repository 생성
- git clone 후
- AWS환경 또는 Docker환경에서의 Ubuntu18.04 환경설정
- 일정 계획
- Pint OS 깃북과 운영체제 교재를 기반으로 개인 공부

### 23.05.27(토)
- Alarm Clock🕰️ 구현 시작
- 목표 : Alarm Clock 구현 완료
- 스케줄링 방식: Busy waiting -> Sleep/Wake up
- 쓰레드 디스크립터 필드 추가(wakeup_tick)
- sleep queue 자료구조 추가
- 전역 변수 추가(next_tick_to_awake)
- thread_init() 함수 수정
- timer_sleep() 함수 수정
- thread_sleep() 함수 구현: 현재 스레드를 sleep queue에 삽입하고 blocked 상태로 만든다. intr_disable() 상태 유지
- timer_interrupt() 함수 수정: sleep queue에서 깨어날 스레드가 있는지 확인한 후 깨운다.
- wakeup() 함수 구현: wake_up값이 인자로 받은 ticks보다 크거나 같은 스레드를 전부 깨운다.
- save_min_tick() 함수 구현: next_tick_to_awake 변수가 깨워야 할 스레드 중 가장 작은 tick을 가지도록 업데이트한다.
- return_min_tick() 함수 구현: next_tick_to_awake 변수의 값을 반환한다.
- 결과
![image](https://github.com/gustjd109/pintos-kaist/assets/45006957/84259118-f4e9-4553-8c85-71906d3d9365)

### 트러블이슈 🗑️ 
- sleep_list에서 깨워야할 스레드를 list_remove()로 제거한 후에 다음 sleep_list 요소를 탐색해야하는데, list_remove()를 사용하면 이미 next가 끊겼기 때문에 계속해서 통과하지 못했다. list_remove()의 반환값이 next임을 확인하고 통과할 수 있었다.
```c
void
wakeup(int64_t mtick) {
	if (list_empty(&sleep_list)) {
		return;
	}
	
	enum intr_level old_level = intr_disable ();
	struct list_elem * curr_elem = list_front(&sleep_list);
	struct thread * curr_thread;
	while (true) {
		curr_thread = list_entry (curr_elem, struct thread, elem);
		ASSERT(curr_thread->status == THREAD_BLOCKED);

		if (curr_thread->wakeup_tick <= mtick) {
			curr_thread->wakeup_tick = NULL;
                        // list_remove(&curr_thread->elem);
                        // curr_thread = list_next(curr_thread);
			curr_elem = list_remove(&curr_thread->elem);
			thread_unblock(curr_thread);
		}else{
			curr_elem = list_next(curr_elem);
		}
		if (curr_elem == list_end(&sleep_list))
			break;
	}

	save_min_tick();
	intr_set_level (old_level);
}
```

### 2023.05.28(일)
- Priority Scheduling🗓️ 개념 공부 시작

### 2023.05.29(월)
- Priority Scheduling🗓️ 구현 시작
- 목표 : 현재 라운드 로빈으로 스케줄링이 되어있는 것을 우선순위를 고려하여 스케줄링하도록 수정한다.
- test_max_priority() 함수 구현
- cmp_priority() 함수 구현
- thread_create() 함수 수정
- thread_set_priority() 함수 수정
- ready_list를 우선순위로 정렬 하기 위해 thread_unblock(), thread_yield() 수정
- test_max_priority() 함수 구현: ready_list에서 우선순위가 가장 높은 스레드와 현재 스레드의 우선순위를 비교한다.
```c
list_insert_ordered(&ready_list, &t->elem, cmp_priority, NULL);

bool
cmp_priority (const struct list_elem *a, const struct list_elem *b, void *aux) {
	struct thread * thread_a = list_entry (a, struct thread, elem);
	struct thread * thread_b = list_entry (b, struct thread, elem);
	return thread_a->priority > thread_b->priority;
}
```

### 트러블이슈 🗑️ 
- 스레드의 우선순위가 바뀔 때에는, 현재 실행 중인 스레드와 ready_list의 첫 번째 스레드의 우선순위를 비교하여 새로 생성된 스레드의 우선순위가 높다면, thread_yield()를 호출하여 CPU를 양보해야 한다. 이걸 안 해줘서 틀렸다.
```c
void
thread_set_priority (int new_priority) {
	thread_current ()->priority = new_priority;
	test_max_priority();
}

void
test_max_priority (void) {
	if (list_empty(&ready_list))
		return;
	if (cmp_priority(list_front(&ready_list), &thread_current()->elem, NULL)) {
		thread_yield();
	}
}
```

### 2023.05.30(화)
- Synchronization 개념 공부 및 구현 시작
- 목표: 여러스레드가 lock, semaphore, condition variable을 얻기위해 기다릴 경우 우선순위가 가장 높은 thread가 CPU를 점유하도록 구현한다.
- sema_down() 함수 수정
- sema_up() 함수 수정
- cond_wait() 함수 수정
- cond_signal() 함수 수정
- cmp_sem_priority() 함수 구현

### 트러블이슈 🗑️ 
1. cmp_sem_priority() 함수 구현 트러블 슈팅
    - cmp_sem_priority() 함수 소스코드
        ```C
        /* semaphore_elem가 나타내는 세마포어의 waiters 리스트의 맨 앞 스레드끼리 우선순위를 비교하는 함수 */
        bool cmp_sem_priority (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED) {
            struct semaphore_elem *sa = list_entry(a, struct semaphore_elem, elem);
            struct semaphore_elem *sb = list_entry(b, struct semaphore_elem, elem);

            struct list *wa = &(sa->semaphore.waiters);
            struct list *wb = &(sb->semaphore.waiters);

            // 세마포어의 waters 리스트는 이미 내림차순으로 정렬되어 있으므로, 각 세마포어의 waiters 리스트의 맨 앞의 elem가 가장 우선순위가 높은 스레드가 됨
            // 사실 각 세마포어에는 스레드가 하나밖에 없어서 굳이 맨 앞의 elem를 찾을 필요 없이 그냥 하나 찾아도 가능
            struct thread *ta = list_entry(list_begin (wa), struct thread, elem);
            struct thread *tb = list_entry(list_begin (wb), struct thread, elem);

            return ta->priority > tb->priority; // 첫 번째 스레드의 우선순위가 두 번째 스레드의 우선순위보다 높으면 1을 반환 낮으면 0을 반환
        }
        ```
        - 이 함수는 그냥 보자마자 어떻게 구현해야 할지 감이 전혀 오지 않아 구현하는 데 가장 어려웠던 함수이다.
        - 한양대학교 핀토스 PDF 자료를 참고하여 기본 구조체 선언 정도만 구현할 수 있었다.
        - 하지만, semaphore, semaphore_elem, condition variables 세 자료구조가 꼬여있어 이해하기 어려웠고, condition 변수 내에서 어떤 방식으로 스레드가 대기하는지 정확한 동작 과정을 알 수 없어 더 이상 구현을 진행하지 못했다.
        - 결국, 새벽 2시쯤 강의실에 다시 나와서 김민석님, 김용현님의 설명을 듣고 함수를 구현할 수 있었다.
        - 더 큰 문제는 다음이다...<br><br>
2. cond_wait()와 cond_signal() 함수 구현 트러블 슈팅
    - cond_wait() 함수 소스코드
        ```C
        void cond_wait (struct condition *cond, struct lock *lock) {
            struct semaphore_elem waiter;

            ASSERT (cond != NULL);
            ASSERT (lock != NULL);
            ASSERT (!intr_context ());
            ASSERT (lock_held_by_current_thread (lock));

            sema_init (&waiter.semaphore, 0);
            list_push_back (&cond->waiters, &waiter.elem);
            //list_insert_ordered (&cond->waiters, &waiter.elem, cmp_sem_priority, NULL);
            lock_release (lock);
            sema_down (&waiter.semaphore);
            lock_acquire (lock);
        }
        ```
        - 원래대로라면 list_push_back() 함수를 주석 처리하고, list_insert_ordered() 함수로 대체해야 한다.
        - 하지만, list_push_back() 함수를 그대로 사용해도 괜찮으며, 코드를 확인해 보자.
        - 우선 semaphore_elem의 waiters의 세마포어 하나의 생성하면서 값을 0으로 초기화해 준다.
        - 이때, 이 세마포어는 스레드를 아직 할당받지 않았으므로 우선순위도 가지고 있지 않은 elem이다.
        - 그래서, list_insert_ordered() 함수를 사용해서 스레드의 우선순위를 비교하는 것은 쓸모없는 동작이며, 여기서 정렬해 줄 필요가 없다.
        - list_push_back() 함수를 그대로 사용하여 cond_waiters 리스트 맨 뒤에 추가시켜 주면 된다.
        - 마지막으로, sema_down() 함수를 이용해서 공유자원 사용을 요청한다.
        - 이때, 세마포어가 우선순위를 가진 스레드가 된다.
        - cond_signal() 함수 소스코드
        ```C
        void cond_signal (struct condition *cond, struct lock *lock UNUSED) {
            ASSERT (cond != NULL);
            ASSERT (lock != NULL);
            ASSERT (!intr_context ());
            ASSERT (lock_held_by_current_thread (lock));

            if (!list_empty (&cond->waiters)) {
                // list_sort(&cond->waiters, cmp_sem_priority, NULL); // 대기 중에 우선순위가 변경되었을 가능성이 있으므로 조건변수 waiters 리스트 재 정렬
                struct list_elem * max_elem = list_min (&cond->waiters, cmp_sem_priority, NULL);
                list_remove (max_elem);
                // sema_up (&list_entry (list_pop_front (&cond->waiters), struct semaphore_elem, elem)->semaphore);
                sema_up (&list_entry (max_elem, struct semaphore_elem, elem)->semaphore);
            }
        }
        ```
        - 정렬은 cond_signal() 함수에서 sema_up을 하기 전에 수행한다.
        - list_sort() 함수를 사용하는 것 보다, list_max() 함수를 이용하여 우선순위 제일 높은 값을 찾는 것이 더 좋다.
        - list_sort()는 시간복잡도가 O(NlogN)이고, list_max() 함수는 O(N)이기 때문이다.
        - sema_up() 함수에서의 list_sort() 함수도 동일하게 코드를 수정해야 한다.
        - list_max() 함수 소스코드
        ```C
        struct list_elem * list_max (struct list *list, list_less_func *less, void *aux) {
            struct list_elem *max = list_begin (list);
            if (max != list_end (list)) {
                struct list_elem *e;

                for (e = list_next (max); e != list_end (list); e = list_next (e))
                    if (less (max, e, aux))
                        max = e;
            }
            return max;
        }
        ```
        - 우선순위 제일 높은 값을 찾을 때 중요한 부분은 list_max() 함수가 아닌, list_min() 함수를 사용해야 한다는 것이다.
        - less 조건(a < b가 참인 경우)을 이용하므로 list_max() 함수를 사용하면 최소값, list_min()을 이용하면 최대값을 찾을 수 있기 때문이다.

### 2023.05.31(수)
- Priority Donation 개념 공부 및 구현 시작
- Priority Inversion, Multiple Donation, Nested Donation 문제를 이해하고 해결한다.
- Priority Inversion: 우선순위가 높은 스레드가 낮은 스레드를 기다리는 현상
- lock_acquire() 함수 수정
- lock_release() 함수 수정
- thread_set_priority() 함수 수정
- donate_priority() 함수 구현
- remove_with_lock() 함수 구현
- refresh_priority() 함수 구현
- struct thread 자료구조에 priority donation 관련 항목을 추가(struct lock *wait_on_lock, struct list donations, struct list_elem donation_elems)
- init_thread() 함수 수정

### 트러블이슈
- 자잘한 오타 이슈, 자잘한 함수 위치 이슈

## 학습내용 🏫 

### 멀티스레딩

멀티스레딩은 컴퓨터 프로그래밍에서 여러 개의 thread를 사용하여 동시에 작업을 수행하는 기술이다. 이를 통해 여러 작업을 동시에 처리하고 병렬성을 이용하여 성능을 향상시킬 수 있다. 멀티스레딩을 구현하기 위해서는 스레드의 생성, 동기화, 스케줄링, 자원 공유 등을 적절하게 관리해야 한다.


#### 멀티스레딩의 이점

1. **성능 향상**: 멀티스레딩은 여러 작업을 동시에 실행함으로써 CPU의 사용률을 높이고 작업을 병렬로 처리함으로써 전체적인 성능을 향상시킬 수 있다.
    
2. **응답성 개선**: 멀티스레딩을 사용하면 작업을 여러 스레드로 분할하여 동시에 처리할 수 있으므로, 사용자에게 빠른 응답성을 제공할 수 있다.
    
3. **자원 공유**: 멀티스레딩은 여러 스레드가 공유된 자원에 동시에 접근하고 사용할 수 있도록 해준다. 이를 통해 효율적인 자원 활용이 가능하다.
    
4. **모듈화와 유연성**: 멀티스레딩은 프로그램을 여러 개의 독립적인 모듈 또는 작업으로 분할할 수 있다. 각 모듈은 별도의 스레드로 실행될 수 있으며, 모듈 간의 상호작용을 통해 유연하고 모듈화된 프로그램을 작성할 수 있다.


#### 멀티스레딩에서의 문제

멀티스레딩 환경에서 발생하는 다양한 문제들이 존재한다. 이러한 문제들은 스레드들이 공유된 자원에 동시에 접근하거나 서로 간섭하는 경우에 발생할 수 있다. 몇 가지 주요한 멀티스레딩 문제들은 다음과 같다.

1. **경쟁 상태([[Race Condition과 Mutual Exclusion]])**: 경쟁 상태는 두 개 이상의 스레드가 공유된 자원에 접근하여 변경하려고 할 때 발생한다. 이때 스레드들이 서로의 실행 순서에 의존하며, 실행 순서에 따라 결과가 달라지는 문제가 발생할 수 있다.
    
2. **교착 상태([[Deadlock]])**: 교착 상태는 두 개 이상의 스레드가 서로가 점유한 자원을 기다리며 무한히 대기하는 상태이다. 각 스레드가 상대방이 점유한 자원을 요청하고 대기하면서 상호 간에 진행이 멈추게 되는 문제가 발생한다.
	
3. **살아있는 교착 상태(Livelock)**: Livelock은 각각의 프로세스나 스레드는 작업을 수행하기 위해 다른 프로세스나 스레드에게 양보하려고 시도하지만, 서로가 계속해서 양보하여 아무런 진전이 없는 상태를 말한다. Livelock은 Deadlock과 비슷한 현상이지만, 프로세스나 스레드들이 움직이는 것처럼 보이는 차이가 있다.
    
4. **우선순위 역전(Priority Inversion)**: 우선순위 역전은 우선순위가 낮은 스레드가 우선순위가 높은 스레드가 점유한 자원을 기다리는 상황에서 발생한다. 이로 인해 우선순위가 높은 스레드의 실행이 지연되는 문제가 발생할 수 있다.
    
5. **스레드 간 통신과 동기화 문제**: 멀티스레드 환경에서 스레드들이 공유된 자원에 안전하게 접근하고 통신하는 것은 도전적인 과제이다. 스레드 간의 동기화 문제, 데이터 일관성 문제, 데드락 등이 발생할 수 있다.
	
6.  **기아 상태(Starvation)**: 기아 상태는 특정 스레드가 필요한 자원 또는 CPU 시간을 무한히 기다리는 상태를 말한다. 다른 스레드들이 지속적으로 자원을 점유하거나 높은 우선순위를 가지는 경우, 낮은 우선순위의 스레드는 필요한 자원에 접근하지 못하거나 CPU 시간을 제한적으로 받게 된다.

### Race Condition 🏃 

- 경쟁 상태
- 여러 스레드가 공유된 자원에 동시에 접근하여 그 순서와 타이밍에 따라 결과가 달라지는 상황을 말한다.
- 이런 경우, 실행할 때마다 다른 결과를 얻는다.
- 컴퓨터의 작동에서 일반적으로 발생하는 **결정적** 결과와 달리, 결과가 어떠할지 알지 못하거나 실행할 때마다 결과가 다른 경우를 **비결정적(Indeterminate)**인 결과라고 부른다.


#### Critical Section

- 임계 영역
- 멀티스레드가 같은 코드를 실행할 때 "경쟁 상태"이 발생하기 때문에, 이러한 코드 부분은 **임계 영역(Critical Section)**이라고 부른다.
- 공유 자원에 접근하지만, 동시에 여러 스레드에서 실행되면 안 되는 코드 부분이다.


#### Mutual Exclusion

- 상호 배제
- 동기화 기법의 하나
- 락이나 세마포어 등을 사용하여 구현할 수 있다.
- 동시에 실행되는 여러 개의 스레드나 프로세스가 공유된 자원에 접근할 때, 한 번에 하나의 스레드  또는 프로세스만이 접근할 수 있도록 제한하는 개념이다.
- 즉, 상호 배제를 통해 여러 스레드가 동시에 같은 자원을 수정하거나 접근하는 것을 방지하여 데이터의 일관성과 정확성을 보장한다.
	- 이를 통해 동시성 문제인 Race Condition을 방지하고, 안전하게 공유 자원을 사용할 수 있다.
- 상호 배제를 구현할 때는 데드락이 발생할 수 있으므로, 이를 고려하여 설계하고 검증해야 한다.


#### 상호 배제 기법

데이터 일관성을 유지하고, 경쟁 상태를 방지하기 위한 상호 배제 기법들이 존재한다.

1. 락(Lock)과 세마포어(Semaphore): 락과 세마포어는 가장 일반적인 상호 배제 메커니즘이다. 락은 단일 스레드만이 특정 자원에 접근할 수 있도록 하는 동기화 객체이다. 세마포어는 동시에 접근 가능한 스레드 수를 제어하여 상호 배제를 달성한다.
    
2. 뮤텍스(Mutex): 뮤텍스는 락의 한 유형으로, 상호 배제를 위해 사용된다. 뮤텍스는 한 번에 하나의 스레드만이 잠금(lock)을 획득하고 접근할 수 있도록 보장한다.
    
3. 원자적 연산(Atomic Operations): 원자적 연산은 하나의 연산으로 인식되어 동시에 접근되는 상황에서 데이터의 일관성과 상호 배제를 보장한다. 원자적 연산은 스레드 간의 경쟁 상태(Race Condition)를 피하는 데 도움이 된다.
    
4. 조건 변수(Condition Variable): 조건 변수는 스레드의 실행 흐름을 제어하고 상호 배제를 구현하는 데 사용된다. 스레드는 조건 변수를 사용하여 특정 조건을 기다리거나 신호를 보내는 방식으로 상호 배제를 달성할 수 있다.

### Scheduler
- 운영체제는 CPU와 같은 컴퓨터 자원들을 적절히 프로세스마다 배분함으로써 효율적으로 많은 프로세스들을 동시에 실행시킬 수 있다. 이런 역할을 수행하는 프로세스를 Scheduler라고 한다.

### Round Robin(RR)
- 선점형 스케줄링의 하나.
- 프로세스들 사이에 우선순위를 두지 않고, 시간단위 순서대로 CPU를 할당하는 방식
- 컴퓨터 자원을 사용할 수 있는 기회를 프로세스들에게 공정하게 부여할 수 있다.
   - 각 프로세스에 일정 시간을 할당하고, 일정 시간이 지나면 다음 프로세스에게 기회를 주고, 또 그 다음 
   프로세스에게 기회를 주는 방식
- 단점 : 수행이 끝난 프로세스는 Queue 끝으로 밀려나기 때문에 기다리는 시간 + 실행 시간은 길어질 수밖에 없다

## 회고
### 강인호
- https://ufo0563.tistory.com/57
- 머릿속에 동작순서,원리를 큰그림으로 그릴 수 있게 공부하기

### 김민석
- https://insengnewbie.tistory.com/348
- 배운 것: 프로세스와 쓰레드의 차이, Race Condition과 Mutual Exclusion, Priority Scheduling, Priority Inversion, Priority Donation, 락, 컨디션 변수, 세마포어
- 코딩할 때는 오탈자 체크를 잘하자.
- 함수 위치를 잘 체크하자.
- 주어진 자료구조를 잘 이해하자.

### 황현성
- https://github.com/gustjd109/TIL/blob/main/KRAFTON%20JUNGLE/WEEK08/WEEK08%20DAY6.md
- 오타로 인해 오류 발생하지 않도록 확인 잘하자 제발...

## 개선점 🐶 

- 초반에는 팀업에 집중해서 개인 공부 후에 코드 구현을 다같이 진행했었는데, 구현하는데 너무 시간이 오래 걸리는 바람에 각자 구현한 후 서로 피드백을 해주기로 했다. 그러나 서로 간의 진도가 달라지는 문제가 발생했고, 다음부터는 서로 간의 진행상황을 적극 공유해야겠다.
- 개념이 중요하더라도 결국 해야하는 것은 구현이기 때문에, 구현에도 집중할 필요성을 느꼈다. list 자료구조를 제대로 살펴보지 않았다가 사소한 에러에 시간을 많이 허비해버렸다.
- 