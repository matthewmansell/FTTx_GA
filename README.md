# FTTx_GA
GA to find optimal rollout plans for fibre to the cabinet.<br/>
Calculated from given area population and potential uptake rates.

## Setup
- Parent selection: Tournament (5 random participants)
- Mutation method: 2 point with validity check
- Mutation chance: 5%
- Crossover method: Random point with rollback correction
- Population size: 3000
- Generations: 150

## Additional Features
- Problem files loader (settings and areas files)
- Results file log

## 'Random Point Crossover With Rollback'
Custom crossover method to avoid array placement bias and maximise crossovers when ensuring valid solution.<br/>
Efficient and progressive compared to unary approach to fault scanning.

1. Decides number of points to crossover.
2. Swaps this number of random points recording each index.
3. If solution is invalid, points are swaped back (rollback) until valid solution reached.
