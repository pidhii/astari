<pre>
<b>pl</b>

Tiny embeddable Prolog interpreter with first-class query continuations.

For a quick-start gude see <a href="https://github.com/pidhii/astari/tree/master/pl/INTRO.md">./pl/INTRO.md</a>.
Builtins are in WIP, see <a href=https://github.com/pidhii/astari/tree/master/pl>./pl/README.md</a> for the status.

Performance:
<table>
  <tr>
    <th> program </th>
    <th> astari / SWI-Prolog <sup>(*)</sup> </th>
  </tr>
  <tr>
    <th> <a href="https://github.com/SWI-Prolog/bench/blob/master/programs/queens_8.pl">queens 11</a> (find all solutions) </th>
    <td> 5 </td>
  </tr>
</table>

<sup>*</sup> CPU time elapsed ($ stari-pl ...) / CPU time elapsed ($ swipl ...)
</pre>

---

<pre>
<b>astari</b>

A framework for combining a CSP solver with ML into an A* algorithm for traversal over complex decision trees.    
</pre>

---

<pre>
<b>opium</b>

Rebirth of my idea about a practical universal constraint-driven type system engine. 
</pre>
