# FTTx_GA
GA to find optimal rollout plans for fibre to the cabinet.

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
Custom crossover method to avoid array placement bias and maximise crossovers when ensuring valid solution.\n
Efficient and progressive compared to unary approach to fault scanning.

- Decides number of points to crossover.
- Swaps this number of random points recording each index.
- If solution is invalid, points are swaped back (rollback) until valid solution reached.
