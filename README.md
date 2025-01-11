# DCCS
02360340 - Project in Computer Communication
In this project we evaluated 3 congestion control protocols.
We used the pre-exsisting implemetnation of DCTCP (https://www.nsnam.org/docs/release/3.31/doxygen/dctcp-example_8cc_source.html) and implemented both DC-Vegas and DCCS.

#contents
1) Papers
   This directory contains the relevant academic papers and our own project report.
3) scratch/Convergence
  This directory contains the code used for our evaluations on the single-rack topology.
  The directory dccs_conv_without_flow_num includes the code relevant to the flow num problem described in the DCCS improvement section of our report.
  The directory defective_dccs_conv includes the code with a too large drain cycle.
5) scratch/Fairness
  This directory contains the code used for our evaluations on the multi-bottleneck topology.
  The directory defective_dctcp_fairness includes the code relevant to the delack problem described in the challenges section of our report (challenge 1).
7) src/internet/model
   This directory contains the source code and headers of the relevant protocols (DCCS, DCTCP, DC-Vegas).
8) src/internet/dccs_makefile 
   This direcory contains our updated CMakeLists.txt file.
   We recommend adding our protocols manually to your own CMakeLists.txt, and not directly copying this file.

#set up
1) Clone and set up ns-3 (follow their official guides).
2) Clone our directory.
3) Recursively copy our directory into "ns-allinone-3.40/ns-3.40/" (the directory that contains scratch and src directories).
4) Add the relevant protocols to "src/internet/CMakeLists.txt".
   For each protocol, you must add the relative paths of both the .cc and .h files.
