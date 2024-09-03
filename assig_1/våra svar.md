# Excercise 1

| Num threads | Array size (MB) | wall time (seconds)|
| -------- | ------- | ------- |
| 1 | 5 | 2.14978 |
| 2 | 5 | 1.09502 |
| 4 | 5 | 0.541949 |
| 8 | 5 | 0.294423 |
| 100 | 5 | 0.174868 |
| 1000 | 5 | 0.210484 |
| 1 | 10 | 4.2529 |
| 2 | 10 | 2.19669 |
| 4 | 10 | 1.10393 |
| 8 | 10 | 0.57636 |
| 100 | 10 | 0.358191 |
| 1000 | 10 | 0.374912 |


The measurements from the table above show that for a small number of threads *T*, the execution time (wall time) approximately gets cut in half as the number of threads double. However, this is only the case for small *T*, as the execution time increases when going from 100 threads to 1000 threads. This is likely due to the overhead induced by thread creation and termination, as well as many threads being created without sufficient data to work on for them to serve a beneficial purpose and only causing the OS to spend extra time on creating and terminating them. 
If we apply Amdahl's law on the measurements for small *T* <= 8, a reasonable deduction would be the program is approximately 100% parallelizable.
1
Furthermore, for *T* > 8 we can derive from the table that 
