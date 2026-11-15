---
id: graphs
title: DSP graphs and latency compensation
description: Connect prepared processors, explicit sidechains and retained transitions.
section: Guides
order: 33
revision: 1
parent: overview
---

## Graph preparation {#topology}

AudioGraph supplies a prepared single-bus DAG. MultiBusGraph adds DspNode, DspBus and DspConnection with explicit bus IDs local to each node. graphInputNode/graphOutputNode identify external boundaries. Preparation copies topology but borrows processor contexts/functions; keep them alive and serialized until the graph is replaced.

MultiBusGraph supports at most 256 nodes, 4096 connections and 32 buses per layout. Cycles are rejected. Declare each node's fixed DspTiming and correct input/output shapes. Sidechains are real input buses rather than hidden main-channel conventions.

## Alignment, errors and transitions {#timing}

Branches entering a node and external outputs align to the greatest declared latency. Bypass keeps the processor warm and uses main bus 0 delayed to its declared latency. Unbounded tail uses SIZE_MAX; finite path tails are bounded by the graph's accepted limits. A callback error silences external output but cannot undo states of earlier nodes.

GraphTransition retains two prepared graphs under a fixed latency budget: stage outside processing, prewarm with live input, then crossfade. Use independent mutable contexts for the two graphs. No graph is destroyed at fade completion; later replacement/destruction occurs outside the callback. Latency stays fixed until new preparation/staging. Frequency-dependent IIR delay and creative pitch effects need an application timing policy beyond integer PDC.

## Public API {#api}

- [cydsp/processors/AudioGraph.h](doc:api-processors-audiograph)
- [cydsp/processors/MultiBusGraph.h](doc:api-processors-multibusgraph)
