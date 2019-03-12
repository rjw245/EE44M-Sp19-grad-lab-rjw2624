# This plots an event log captured from the OS

import matplotlib.pyplot as plt
import numpy as np
import re
import string

example = """\
Name: Interpreter  Time: 705617827 cycles  Type: FG START
Name: DAS  Time: 705643651 cycles  Type: PT START
Name: DAS  Time: 705644083 cycles  Type: PT END
Name: DAS  Time: 705683651 cycles  Type: PT START
Name: DAS  Time: 705684083 cycles  Type: PT END
Name: ADC  Time: 705686869 cycles  Type: PT START
Name: PID  Time: 705687561 cycles  Type: FG START
Name: ADC  Time: 705687919 cycles  Type: PT END
Name: IdleTask  Time: 705688217 cycles  Type: FG START
Name: Consumer  Time: 705689135 cycles  Type: FG START
Name: IdleTask  Time: 705689471 cycles  Type: FG START
Name: DAS  Time: 705723651 cycles  Type: PT START
Name: DAS  Time: 705724085 cycles  Type: PT END
Name: DAS  Time: 705763651 cycles  Type: PT START
Name: DAS  Time: 705764083 cycles  Type: PT END
Name: DAS  Time: 705803651 cycles  Type: PT START
Name: DAS  Time: 705804083 cycles  Type: PT END
Name: DAS  Time: 705843651 cycles  Type: PT START
Name: DAS  Time: 705844085 cycles  Type: PT END
Name: PID  Time: 705852763 cycles  Type: FG START
Name: IdleTask  Time: 705853105 cycles  Type: FG START
Name: DAS  Time: 705883651 cycles  Type: PT START
Name: DAS  Time: 705884083 cycles  Type: PT END
Name: ADC  Time: 705887189 cycles  Type: PT START
Name: PID  Time: 705887881 cycles  Type: FG START
Name: ADC  Time: 705888239 cycles  Type: PT END
Name: IdleTask  Time: 705888537 cycles  Type: FG START
Name: Consumer  Time: 705889455 cycles  Type: FG START
Name: IdleTask  Time: 705889791 cycles  Type: FG START
Name: DAS  Time: 705923651 cycles  Type: PT START
Name: DAS  Time: 705924083 cycles  Type: PT END
Name: DAS  Time: 705963651 cycles  Type: PT START
Name: DAS  Time: 705964085 cycles  Type: PT END
Name: DAS  Time: 706003651 cycles  Type: PT START
Name: DAS  Time: 706004083 cycles  Type: PT END
Name: DAS  Time: 706043651 cycles  Type: PT START
Name: DAS  Time: 706044083 cycles  Type: PT END
Name: PID  Time: 706053081 cycles  Type: FG START
Name: IdleTask  Time: 706053423 cycles  Type: FG START
Name: DAS  Time: 706083651 cycles  Type: PT START
Name: DAS  Time: 706084085 cycles  Type: PT END
Name: ADC  Time: 706086869 cycles  Type: PT START
Name: PID  Time: 706087561 cycles  Type: FG START
Name: ADC  Time: 706087919 cycles  Type: PT END
Name: IdleTask  Time: 706088217 cycles  Type: FG START
Name: Consumer  Time: 706089135 cycles  Type: FG START
Name: IdleTask  Time: 706089471 cycles  Type: FG START
Name: DAS  Time: 706123651 cycles  Type: PT START
Name: DAS  Time: 706124083 cycles  Type: PT END
Name: DAS  Time: 706163651 cycles  Type: PT START
Name: DAS  Time: 706164083 cycles  Type: PT END
Name: DAS  Time: 706203651 cycles  Type: PT START
Name: DAS  Time: 706204085 cycles  Type: PT END
Name: DAS  Time: 706243651 cycles  Type: PT START
Name: DAS  Time: 706244083 cycles  Type: PT END
Name: PID  Time: 706252759 cycles  Type: FG START
Name: IdleTask  Time: 706253101 cycles  Type: FG START
Name: DAS  Time: 706283651 cycles  Type: PT START
Name: DAS  Time: 706284083 cycles  Type: PT END
Name: ADC  Time: 706287189 cycles  Type: PT START
Name: PID  Time: 706287881 cycles  Type: FG START
Name: ADC  Time: 706288239 cycles  Type: PT END
Name: IdleTask  Time: 706288537 cycles  Type: FG START
Name: Consumer  Time: 706289455 cycles  Type: FG START
Name: IdleTask  Time: 706289791 cycles  Type: FG START
Name: DAS  Time: 706323651 cycles  Type: PT START
Name: DAS  Time: 706324085 cycles  Type: PT END
Name: DAS  Time: 706363651 cycles  Type: PT START
Name: DAS  Time: 706364083 cycles  Type: PT END
Name: DAS  Time: 706403651 cycles  Type: PT START
Name: DAS  Time: 706404083 cycles  Type: PT END
Name: DAS  Time: 706443651 cycles  Type: PT START
Name: DAS  Time: 706444085 cycles  Type: PT END
Name: PID  Time: 706453081 cycles  Type: FG START
Name: IdleTask  Time: 706453423 cycles  Type: FG START
Name: DAS  Time: 706483651 cycles  Type: PT START
Name: DAS  Time: 706484083 cycles  Type: PT END
Name: ADC  Time: 706486869 cycles  Type: PT START
Name: PID  Time: 706487561 cycles  Type: FG START
Name: ADC  Time: 706487919 cycles  Type: PT END
Name: IdleTask  Time: 706488217 cycles  Type: FG START
Name: Consumer  Time: 706489135 cycles  Type: FG START
Name: IdleTask  Time: 706489471 cycles  Type: FG START
Name: DAS  Time: 706523651 cycles  Type: PT START
Name: DAS  Time: 706524083 cycles  Type: PT END
Name: DAS  Time: 706563651 cycles  Type: PT START
Name: DAS  Time: 706564085 cycles  Type: PT END
Name: DAS  Time: 706603651 cycles  Type: PT START
Name: DAS  Time: 706604083 cycles  Type: PT END
Name: DAS  Time: 706643651 cycles  Type: PT START
Name: DAS  Time: 706644083 cycles  Type: PT END
Name: PID  Time: 706652759 cycles  Type: FG START
Name: IdleTask  Time: 706653101 cycles  Type: FG START
Name: DAS  Time: 706683651 cycles  Type: PT START
Name: DAS  Time: 706684085 cycles  Type: PT END
Name: ADC  Time: 706687189 cycles  Type: PT START
Name: PID  Time: 706687881 cycles  Type: FG START
Name: ADC  Time: 706688239 cycles  Type: PT END
Name: IdleTask  Time: 706688537 cycles  Type: FG START
Name: Consumer  Time: 706689455 cycles  Type: FG START"""
# """\
# Name: Consumer  Time: 51481744 cycles  Type: FG START
# Name: IdleTask  Time: 51482072 cycles  Type: FG START
# Name: DAS  Time: 51519048 cycles  Type: PT START
# Name: DAS  Time: 51519478 cycles  Type: PT END
# Name: DAS  Time: 51559070 cycles  Type: PT START
# Name: DAS  Time: 51559500 cycles  Type: PT END
# Name: DAS  Time: 51599070 cycles  Type: PT START
# Name: DAS  Time: 51599502 cycles  Type: PT END
# Name: DAS  Time: 51639070 cycles  Type: PT START
# Name: DAS  Time: 51639500 cycles  Type: PT END
# Name: Display  Time: 51645328 cycles  Type: FG START
# Name: IdleTask  Time: 51645664 cycles  Type: FG START
# Name: DAS  Time: 51679070 cycles  Type: PT START
# Name: DAS  Time: 51679500 cycles  Type: PT END
# Name: DAS  Time: 51719070 cycles  Type: PT START
# Name: DAS  Time: 51719502 cycles  Type: PT END
# Name: DAS  Time: 51759070 cycles  Type: PT START
# Name: DAS  Time: 51759500 cycles  Type: PT END
# Name: DAS  Time: 51799070 cycles  Type: PT START
# Name: DAS  Time: 51799500 cycles  Type: PT END
# Name: Display  Time: 51808942 cycles  Type: FG START
# Name: IdleTask  Time: 51809278 cycles  Type: FG START
# Name: DAS  Time: 51839144 cycles  Type: PT START
# Name: DAS  Time: 51839576 cycles  Type: PT END
# Name: DAS  Time: 51879068 cycles  Type: PT START
# Name: DAS  Time: 51879498 cycles  Type: PT END
# Name: DAS  Time: 51919070 cycles  Type: PT START
# Name: DAS  Time: 51919500 cycles  Type: PT END
# Name: DAS  Time: 51959070 cycles  Type: PT START
# Name: DAS  Time: 51959502 cycles  Type: PT END
# Name: Display  Time: 51972562 cycles  Type: FG START
# Name: IdleTask  Time: 51972898 cycles  Type: FG START
# Name: DAS  Time: 51999070 cycles  Type: PT START
# Name: DAS  Time: 51999500 cycles  Type: PT END
# Name: DAS  Time: 52039070 cycles  Type: PT START
# Name: DAS  Time: 52039500 cycles  Type: PT END
# Name: DAS  Time: 52079084 cycles  Type: PT START
# Name: DAS  Time: 52079516 cycles  Type: PT END
# Name: DAS  Time: 52119068 cycles  Type: PT START
# Name: DAS  Time: 52119498 cycles  Type: PT END
# Name: Display  Time: 52136174 cycles  Type: FG START
# Name: IdleTask  Time: 52136510 cycles  Type: FG START
# Name: DAS  Time: 52159070 cycles  Type: PT START
# Name: DAS  Time: 52159500 cycles  Type: PT END
# Name: DAS  Time: 52199070 cycles  Type: PT START
# Name: DAS  Time: 52199502 cycles  Type: PT END
# Name: DAS  Time: 52239070 cycles  Type: PT START
# Name: DAS  Time: 52239500 cycles  Type: PT END
# Name: DAS  Time: 52279070 cycles  Type: PT START
# Name: DAS  Time: 52279500 cycles  Type: PT END
# Name: Display  Time: 52299790 cycles  Type: FG START
# Name: IdleTask  Time: 52300126 cycles  Type: FG START
# Name: DAS  Time: 52319070 cycles  Type: PT START
# Name: DAS  Time: 52319502 cycles  Type: PT END
# Name: DAS  Time: 52359070 cycles  Type: PT START
# Name: DAS  Time: 52359500 cycles  Type: PT END
# Name: DAS  Time: 52399070 cycles  Type: PT START
# Name: DAS  Time: 52399500 cycles  Type: PT END
# Name: DAS  Time: 52439070 cycles  Type: PT START
# Name: DAS  Time: 52439502 cycles  Type: PT END
# Name: Display  Time: 52463408 cycles  Type: FG START
# Name: IdleTask  Time: 52463744 cycles  Type: FG START
# Name: DAS  Time: 52479102 cycles  Type: PT START
# Name: DAS  Time: 52479532 cycles  Type: PT END
# Name: DAS  Time: 52519068 cycles  Type: PT START
# Name: DAS  Time: 52519498 cycles  Type: PT END
# Name: DAS  Time: 52559070 cycles  Type: PT START
# Name: DAS  Time: 52559502 cycles  Type: PT END
# Name: DAS  Time: 52599070 cycles  Type: PT START
# Name: DAS  Time: 52599500 cycles  Type: PT END
# Name: Display  Time: 52627088 cycles  Type: FG START
# Name: IdleTask  Time: 52627424 cycles  Type: FG START
# Name: DAS  Time: 52639070 cycles  Type: PT START
# Name: DAS  Time: 52639500 cycles  Type: PT END
# Name: DAS  Time: 52679070 cycles  Type: PT START
# Name: DAS  Time: 52679502 cycles  Type: PT END
# Name: DAS  Time: 52719070 cycles  Type: PT START
# Name: DAS  Time: 52719500 cycles  Type: PT END
# Name: DAS  Time: 52759070 cycles  Type: PT START
# Name: DAS  Time: 52759500 cycles  Type: PT END
# Name: Display  Time: 52790702 cycles  Type: FG START
# Name: IdleTask  Time: 52791038 cycles  Type: FG START
# Name: DAS  Time: 52799070 cycles  Type: PT START
# Name: DAS  Time: 52799502 cycles  Type: PT END
# Name: DAS  Time: 52839078 cycles  Type: PT START
# Name: DAS  Time: 52839508 cycles  Type: PT END
# Name: DAS  Time: 52879068 cycles  Type: PT START
# Name: DAS  Time: 52879498 cycles  Type: PT END
# Name: DAS  Time: 52919070 cycles  Type: PT START
# Name: DAS  Time: 52919502 cycles  Type: PT END
# Name: Display  Time: 52954318 cycles  Type: FG START
# Name: IdleTask  Time: 52954654 cycles  Type: FG START
# Name: DAS  Time: 52959070 cycles  Type: PT START
# Name: DAS  Time: 52959500 cycles  Type: PT END
# Name: DAS  Time: 52999070 cycles  Type: PT START
# Name: DAS  Time: 52999500 cycles  Type: PT END
# Name: Display  Time: 53030346 cycles  Type: FG START
# Name: IdleTask  Time: 53030674 cycles  Type: FG START
# Name: Consumer  Time: 53033826 cycles  Type: FG START
# Name: IdleTask  Time: 53034154 cycles  Type: FG START"""

log_re = re.compile(r"Name: (\w+)  Time: (\d+) cycles  Type: (FG START|PT START|PT END)")

events = []

for line in example.splitlines():
  match = log_re.match(line)
  if match:
    match = match.groups()
    time_ms = int(match[1])/80000.0
    events.append((match[0], time_ms, match[2]))

task_names = list(set([e[0] for e in events]))

events = [[task_names.index(e[0]), e[1], e[2]] for e in events]

fg_events = filter(lambda e: e[2].startswith("FG") or e[2] == "PT END", events)
bg_events = filter(lambda e: e[2] == "PT START", events)

stack = []
for i, e in enumerate(events):
  if e[2] == "PT START":
    stack.append(events[i-1][0])
  if e[2] == "PT END":
    events[i][0] = stack.pop()

events = sorted(fg_events+bg_events, key=lambda x: x[1])

print events

x = np.array([e[1] for e in events])
time_diffs = [j-i for i, j in zip(x[:-1], x[1:])]
time_diffs.append(time_diffs[-1])
time_diffs = np.array(time_diffs)
y = np.array([e[0] for e in events])

labels = np.array(task_names)
plt.barh(y, time_diffs, left=x, color = 'red', edgecolor = 'red', align='center', height=1)
plt.ylim(max(y)+0.5, min(y)-0.5)
plt.yticks(np.arange(y.max()+1), labels)
plt.xlim(min(x)-1, max(x)+1)
plt.xlabel("Time (ms)")
plt.ylabel("Task")
plt.show()