#pragma comment(linker,"/STACK:256000000")

#ifndef _gencnfformula_h_
#define _gencnfformula_h_

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>

using namespace std;

int xorTypeVals[] = {6, 9}; 
int andTypeVals[] = {1, 2, 4, 8, 14, 13, 11, 7}; 	
set<int> xorType(xorTypeVals, xorTypeVals + 2); 
set<int> andType(andTypeVals, andTypeVals + 8);
int unused[] = {0, 3, 5, 10, 12, 15, 6, 9};
int unused_size = 6;

class Clause {
public:
  	vector<int> myLiterals;

  	void addLiteral(int var, bool isTrue) {
		if (isTrue) {
			myLiterals.push_back(var);
		} else {
			myLiterals.push_back(-var);
		}
	}

	string toString() {
		ostringstream sb;
		for (unsigned int i = 0; i < myLiterals.size(); i++) {
			int cur = myLiterals[i];
			if (cur < 0) {
		  		sb << "~";
			}
	  		sb << cur << " ^ ";
		}
		sb << endl;
		return sb.str();
	}	

	string toSolverString() {
		ostringstream result;
		for (unsigned int i = 0; i < myLiterals.size(); i++) {
			result << myLiterals[i] << " ";
		}
		result << "0" << endl;
		return result.str();
	}
};


class CircuitToSAT {
	//Number of inputs of a circuit
	int myN;

	//Number of gates in the circuit
	int myM;
	
	//Number of outputs in the circuit
	int myOuts;

  	/**
	 * The truth tables for all outputs of the circuit.
	 * 0 and 1 used in usial sence;
	 * value 2 mean that such input is not fixed (circuit can output 0 or 1
	 * for this input). 
	 */
	vector<vector<int> > myValues;
 
	/**
	 * List of circuit encoding. We use variables of 4 types parametraced by at
	 * most 3 integers.
	 *
	 * For more information see the comments in constructor.
     */
	vector<pair<int, vector<int> > > myVariablesList;
   	
  	/**
	 * The mapping from circuit enconding to variables of CNF formula.
	 */
	map< pair<int, vector<int> >, int> myVariables;
  
  	/**
	 * The CNF formula.
	 */
	vector<Clause> myFormula;

	
	string VarToString(pair<int, vector<int> > myVar) {
		ostringstream out;
		vector<int> myIndices = myVar.second;
		out << myVar.first << " : ";
		for (unsigned int i = 0; i < myIndices.size(); i++ ) {
	  		out << myIndices[i] << ", ";
		}
		return out.str();
  	}
  
	/**
	 * This method maps circuit encoding to CNF formula variables.
	 */
  	int Variable(int type, vector<int> indices) {
		int myN;
		pair<int, vector<int> > myVar = make_pair(type, indices);
		if (myVariables.find(myVar) == myVariables.end()) {
			myN = myVariables.size() + 1;
			myVariablesList.push_back(myVar);
			myVariables.insert(make_pair(myVar, myN));
		} else {
			myN = myVariables[myVar];
		}
		return myN;
  	}

	void prohibitGateType(int gate, int gateType, Clause & c) {
		for (int l = 0; l < 4; l++) {
			c.addLiteral(Variable(0, get3List(gate, l / 2, l % 2)), ((gateType & (1 << l)) == 0));
		}
	}

	void addDirectInputRestriction(int maskGate, int input, int inputGate) {
		for (int i = myN; i < myN + myM; i++) {
			Clause c;
			c.addLiteral(Variable(4, get2List(i, maskGate)), false);
			c.addLiteral(Variable(1, get3List(i, input, inputGate)), true);
			myFormula.push_back(c);
		}
	}

	void addMaskInputRestriction(int maskGate, int maskInputGate) {
		for (int i = myN; i < myN + myM; i++) {
			for (int j = myN; j < i; j++) {
				Clause c;
				c.addLiteral(Variable(4, get2List(i, maskGate)), false);
				c.addLiteral(Variable(4, get2List(j, maskInputGate)), false);
				for (int k = 0; k < 2; k++) {
					c.addLiteral(Variable(1, get3List(i, k, j)), true);
				}
				myFormula.push_back(c);
			}
			for (int j = i + 1; j < myN + myM; j++) {
				Clause c;
				c.addLiteral(Variable(4, get2List(i, maskGate)), false);
				c.addLiteral(Variable(4, get2List(j, maskInputGate)), false);
				myFormula.push_back(c);
			}
		}
	}

	void addBottleneck() {
		for (int j = 0; j < 5; j++) {
			Clause cl;
			for (int i = myN; i < myN + myM; i++) {
				cl.addLiteral(Variable(4, get2List(i, j)), true);
				for (int k = myN; k < i; k++) {
					Clause c;
					c.addLiteral(Variable(4, get2List(i, j)), false);
					c.addLiteral(Variable(4, get2List(k, j)), false);
					myFormula.push_back(c);
				}
			}
			myFormula.push_back(cl);
		}
	
		for (int i = myN; i < myN + myM; i++) {
			for (int j = 0; j < 5; j++) {
				for (int k = 0; k < j; k++) {
					Clause c;
					c.addLiteral(Variable(4, get2List(i, j)), false);
					c.addLiteral(Variable(4, get2List(i, k)), false);
					myFormula.push_back(c);
				}
			}
		}

		for (int j = 0; j < 5; j++) {
			for (int i = myN; i < myN + myM; i++) {
				for (int k = 0; k < 16; k++) {
					Clause c;
					c.addLiteral(Variable(4, get2List(i, j)), false);
					if (((j == 1 || j == 2) && andType.count(k) == 0) || ((j == 0 || j == 3 || j == 4) && xorType.count(k) == 0)) {
						for (int l = 0; l < 4; l++) {
							c.addLiteral(Variable(0, get3List(i, l / 2, l % 2)), ((k & (1 << l)) == 0));
						}
					} else {
						continue;
					}
					myFormula.push_back(c);
				} 
			}
		}
	
		for (int j = 0; j < 5; j++) {
			if (j == 2 || j == 4) {
				continue;
			}
			for (int i = myN; i < myN + myM; i++) {
				for (int i1 = myN; i1 < myN + myM; i1++) {
					for (int k1 = 0; k1 < 2; k1++) {
						for (int i2 = myN; i2 < i1; i2++) {
							for (int k2 = 0; k2 < 2; k2++) {
								Clause c;
								c.addLiteral(Variable(4, get2List(i, j)), false);
								c.addLiteral(Variable(1, get3List(i1, k1, i)), false);
								c.addLiteral(Variable(1, get3List(i2, k2, i)), false);
								myFormula.push_back(c);
							}
						}
					}
				}
			}
		}

		addDirectInputRestriction(0, 0, 0);
		addDirectInputRestriction(3, 0, 0);
		addDirectInputRestriction(3, 1, 1);

		addMaskInputRestriction(1, 0);
		addMaskInputRestriction(2, 1);
		addMaskInputRestriction(4, 3);
	
	}

	void addNonKillWithOneAssignment() {
		for (int i = 0; i < 16; i++) {
			if (!andType.count(i)) {
				continue;
			}
			for (int i1 = myN; i1 < myN + myM; i1++) {
				for (int k1 = 0; k1 < 2; k1++) {
					for (int i2 = myN; i2 < myN + myM; i2++) {
						if (i1 == i2) {
							continue;
						}
						for (int k2 = 0; k2 < 2; k2++) {
							for (int j = 0; j < myN; j++) {
								Clause c;
								for (int l = 0; l < 4; l++) {
									c.addLiteral(Variable(0, get3List(i1, l / 2, l % 2)), ((i & (1 << l)) == 0));
								}
								c.addLiteral(Variable(1, get3List(i1, k1, j)), false);
								c.addLiteral(Variable(1, get3List(i2, k2, j)), false);
								myFormula.push_back(c);
							}
						}
					}
				}
			}
		}
	}

	void addSomeStrangeCondition() {
		for (int v1 = 0; v1 < myN; v1++) {
			for (int v2 = v1 + 1; v2 < myN; v2++) {
				for (int i = myN; i < myN + myM; i++) {
					for (int i1 = i + 1; i1 < myN + myM; i1++) {
						for (int k1 = 0; k1 < 2; k1++) {
							for (int i2 = i1 + 1; i2 < myN + myM; i2++) {
								for (int k2 = 0; k2 < 2; k2++) {
									for (int j = 0; j < 16; j++) {
										if (!xorType.count(j)) {
											continue;
										}
										Clause c;
										for (int l = 0; l < 4; l++) {
											c.addLiteral(Variable(0, get3List(i, l / 2, l % 2)), ((j & (1 << l)) == 0));
										}
										c.addLiteral(Variable(1, get3List(i, 0, v1)), false);
										c.addLiteral(Variable(1, get3List(i, 1, v2)), false);
										c.addLiteral(Variable(1, get3List(i1, k1, i)), false);
										c.addLiteral(Variable(1, get3List(i2, k2, i)), false);
										myFormula.push_back(c);
									}
								}
							}
							for (int j = 0; j < 16; j++) {
								if (!xorType.count(j)) {
									continue;
								}
								Clause c;
								c.addLiteral(Variable(1, get3List(i, 0, v1)), false);
								c.addLiteral(Variable(1, get3List(i, 1, v2)), false);
								for (int l = 0; l < 4; l++) {
									c.addLiteral(Variable(0, get3List(i, l / 2, l % 2)), ((j & (1 << l)) == 0));
								}
								c.addLiteral(Variable(1, get3List(i1, k1, i)), false);
								for (int jj = 0; jj < 16; jj++) {
									if (!andType.count(jj)) {
										continue;
									}
									for (int ll = 0; ll < 4; ll++) {
										c.addLiteral(Variable(0, get3List(i1, ll / 2, ll % 2)), ((jj & (1 << ll)) == 0));
									}
									myFormula.push_back(c);
									for (int ll = 0; ll < 4; ll++) {
										c.myLiterals.pop_back();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	void addOutDegreeRestriction() {
		/**
		 *
		 *	This restricts configurations to only those where every gate is used no more than 2 times as input of other gates
		 *
		 *
		 */

		
		for (int j = 0; j < myN + myM; j++) {
			for (int i1 = myN; i1 < myN + myM; i1++) {
				for (int k1 = 0; k1 < 2; k1++) {
					for (int i2 = myN; i2 < myN + myM; i2++) {
						for (int k2 = 0; k2 < 2; k2++) {
							for (int i3 = myN; i3 < myN + myM; i3++) {
								for (int k3 = 0; k3 < 2; k3++) {
									if (!((i1 * 2 + k1 < i2 * 2 + k2) && (i2 * 2 + k2 < i3 * 2 + k3))) {
										continue;
									}
									Clause *cl = new Clause();
									cl->addLiteral(Variable(1, get3List(i1, k1, j)), false);
									cl->addLiteral(Variable(1, get3List(i2, k2, j)), false);
									cl->addLiteral(Variable(1, get3List(i3, k3, j)), false);
									myFormula.push_back(*cl);
								}
							}
						}
					}
				}
			}
		}

	}

	void addOneOutRestriction(int k) {
		for (int i = myN; i < myN + myM; i++) {
			for (int j = myN; j < myN + myM; j++) {
				if (i == j) {
					continue;
				}
				for (int ki = 0; ki < 2; ki++) {
					for (int kj = 0; kj < 2; kj++) {
						Clause cl;
						cl.addLiteral(Variable(1, get3List(i, ki, k)), false);
						cl.addLiteral(Variable(1, get3List(j, kj, k)), false);
						myFormula.push_back(cl);
					}
				}
			}
		}
	}

	void addOutputOrderRestriction() {
		/**
		 *
		 *	This restriction says that first output should be in the gate before last and the second in the last gate
		 *
		 */
		for (int k = 0; k < myOuts; k++) {
			Clause *cl = new Clause();
			cl->addLiteral(Variable(3, get2List(myN + myM - myOuts + k, k)), true);
			myFormula.push_back(*cl);
		}
	}
	
		
	void addLinearStructureRestriction() {	
		/**
		 *
		 *	This restricts configurations to only those that have second input of i-th gate connected to (i-1)-th gate
		 *
		 */
		
		for (int i = myN; i < myN + myM; i++) {
			Clause *cl = new Clause();
			cl->addLiteral(Variable(1, get3List(i, 1, i - 1)), true);
			myFormula.push_back(*cl);
		}
	}

	void addSymmetryRestriction(int synInputs) {
		/**
		 *
		 *	Here starts code, describing symmetry of the first ? gates
		 *
		 */

		for (int i = 1; i < synInputs; i++) {
			for (int j = myN; j < myN + myM; j++) {
				Clause *cl = new Clause();
				for (int k = myN; k < j; k++) {
					cl->addLiteral(Variable(1, get3List(k, 0, i - 1)), true);
				}
				cl->addLiteral(Variable(1, get3List(j, 0, i)), false);
				myFormula.push_back(*cl);
			}
		}
		
		/**
		 *
		 *	Here ends code, describing symmetry of the first (n - 2) gates
		 *
		 */
	}

	void addTopGateTypeRestriction() {
		for (int i = myN; i < myM; i++) {
			for (int j = 0; j < myN - 2; j++) {
				for (int k = 0; k < 2; k++) {
					for (int l = 0; l < 16; l++) {
						if (andType.count(l)) {				
							Clause c;
							c.addLiteral(Variable(1, get3List(i, j, k)), false);
							prohibitGateType(i, l, c);
							myFormula.push_back(c);
						}
					}
				}
			}
		}
	}

	void limitToU2Basis() {
		for (int i = myN; i < myN + myM; i++) {
			for (set<int> :: iterator it = xorType.begin(); it != xorType.end(); it++) {
				Clause c;
				prohibitGateType(i, *it, c);	
				myFormula.push_back(c);
			}
		}
	}

public:
	CircuitToSAT(string file_name) {
		freopen(file_name.c_str(), "r", stdin);	
	
		scanf("%d", &myN);
		scanf("%d", &myM);
		scanf("%d", &myOuts);

		/**
		 *
		 *	These clauses describe configurations where second input of i-th gate is connected to (i-1)-th gate
		 *	!!! This limits full search to only some special configurations !!!
		 *
		 */

//		limitToU2Basis();
//		addLinearStructureRestriction();
//		addOutDegreeRestriction();
//		addSymmetryRestriction(myN);	
//		addOutputOrderRestriction();
//		addTopGateTypeRestriction();

/*		Clause *cl = new Clause();
		cl->addLiteral(Variable(3, get2List(myN + myM - 1, 0)), false);
		myFormula.push_back(*cl);
		
		cl = new Clause();
		cl->addLiteral(Variable(3, get2List(myN + myM - 3, 1)), false);
		myFormula.push_back(*cl);
*/		
		/**
		 *
		 *	This restrictions says that type of the last gate should be 4
		 *
		 */

/*		Clause *cl = new Clause();
		cl->addLiteral(Variable(0, get3List(myN + myM - 1, 0, 0)), false);
		myFormula.push_back(*cl);
	
		cl = new Clause();
		cl->addLiteral(Variable(0, get3List(myN + myM - 1, 0, 1)), false);
		myFormula.push_back(*cl);
	
		cl = new Clause();
		cl->addLiteral(Variable(0, get3List(myN + myM - 1, 1, 0)), true);
		myFormula.push_back(*cl);
	
		cl = new Clause();
		cl->addLiteral(Variable(0, get3List(myN + myM - 1, 1, 1)), false);
		myFormula.push_back(*cl);
	
*/		
		/**
		 *
		 * 	Here ends code that limits bruteforce over combinations
		 *
		 */
		
		
		for (int k = 0; k < myOuts; k++) {
			vector<int> a;
			for (int i = 0; i < (1 << myN); i++) {
				int value;
				if (scanf("%d", &value) != 1) {
					cerr << "Error!!!\n Too short input. Should be = ";
					cerr << myOuts*(1 << myN);
				}
				a.push_back(value);
			}
			myValues.push_back(a);
		}
		
		/* check size */
		int size;
		if(scanf("%d", &size) != 1) {
			size = 0;
		}
		//cerr << size << endl;
		
		for (int i = 0; i < size; i++) {
			int gate_num, first_in, second_in, type;
			scanf("%d: (%d, %d, %d)\n", &gate_num, &first_in, &second_in, &type);
			Clause *c1 = new Clause();
			c1->addLiteral(Variable(1, get3List(gate_num-1, 0, first_in-1)), true);
			myFormula.push_back(*c1);
			
			Clause *c2 = new Clause();
			c2->addLiteral(Variable(1, get3List(gate_num-1, 1, second_in-1)), true);
			myFormula.push_back(*c2);
			
			for (int k = 0; k < 4; k++) {
				Clause *c = new Clause();
				c->addLiteral(Variable(0, get3List(gate_num-1, k / 2, k % 2)), (type >> k) % 2);
				myFormula.push_back(*c);
			}
		}

		for (int i = 0; i < size - 1; i++) {
			addOneOutRestriction(myN + i);
		}
		
		fclose(stdin);
	
		for (int i = myN; i < myN + myM; i++) {
			for (int k = 0; k < 2; k++) {
				Clause *c = new Clause();
				for (int j = 0; j < i; j++) {
					c->addLiteral(Variable(1, get3List(i, k, j)), true);
				}
				/**
				 * type 1 variables are true when j-gate is the k-th input of i-gate;
				 * 
				 * The clause encodes that the i-gate has an k-input from one of the 
				 * previous gates.
				 */ 
				myFormula.push_back(*c);
				for (int j1 = 0; j1 < i; j1++) {
					for (int j2 = 0; j2 < i; j2++) {
						if (j1 == j2) {
							continue;
						}
						Clause *cl = new Clause();
						cl->addLiteral(Variable(1, get3List(i, k, j1)), false);
						cl->addLiteral(Variable(1, get3List(i, k, j2)), false);
						/**
						 * This clause encodes that both j1 and j2-gates cannot be the 
						 * k-input of i-gate together. 
						 */
						myFormula.push_back(*cl);
					}
				}
			}
			for (int j1 = 0; j1 < i; j1++) {
				// we do not need gates that have same gates as both inputs, so we
				// starts from j2=j1
				for (int j2 = j1; j2 < i; j2++) {
					Clause *cl = new Clause();
					cl->addLiteral(Variable(1, get3List(i, 1, j1)), false);
					cl->addLiteral(Variable(1, get3List(i, 0, j2)), false);
					/**
					 * These clauses describe the fact that the first input is connected 
					 * to the gate with smaller number than the second.
					 */
					myFormula.push_back(*cl);

					// We dont need degenerate (in sence of [Zwick91]) gates.
					for (int j = j2 + 1; j < i; j++) {
						Clause *cl1 = new Clause();
						cl1->addLiteral(Variable(1, get3List(j, 0, j1)), false);
						cl1->addLiteral(Variable(1, get3List(j, 1, j2)), false);
						cl1->addLiteral(Variable(1, get3List(i, 0, j1)), false);
						cl1->addLiteral(Variable(1, get3List(i, 1, j)), false);
						myFormula.push_back(*cl1);

						Clause *cl2 = new Clause();
						cl2->addLiteral(Variable(1, get3List(j, 0, j1)), false);
						cl2->addLiteral(Variable(1, get3List(j, 1, j2)), false);
						cl2->addLiteral(Variable(1, get3List(i, 0, j2)), false);
						cl2->addLiteral(Variable(1, get3List(i, 1, j)), false);
						myFormula.push_back(*cl2);

					}
				}
			}
			
			/**
			 *
			 * These clauses describe the fact that the inputs of the gate with 
			 * smaller number are connected to the inputs with smaller numbers.
			 * Here the word smaller means something not trivial, see code for 
			 * explanation.
			 *
			 */
			
			if (i == myN) {
				continue;
			}
			
			for (int j1 = 0; j1 < i; j1++) {
				for (int j2 = j1 + 1; j2 < j1; j2++) {
					Clause *cl = new Clause();
					cl->addLiteral(Variable(1, get3List(i, 1, j1)), false);
					cl->addLiteral(Variable(1, get3List(i - 1, 1, j2)), false);
					myFormula.push_back(*cl);
				}
			}

			for (int j1 = 0; j1 < i; j1++) {
				for (int j2 = 0; j2 < j1; j2++) {
					for (int j = j2 + 1; j < j1; j++) {
						Clause *cl = new Clause();
						cl->addLiteral(Variable(1, get3List(i, 1, j1)), false);
						cl->addLiteral(Variable(1, get3List(i, 0, j2)), false);
						cl->addLiteral(Variable(1, get3List(i - 1, 1, j1)), false);
						cl->addLiteral(Variable(1, get3List(i - 1, 0, j)), false);
						myFormula.push_back(*cl);
					}
				}
			}	
		}
	
		for (int i = myN; i < myN + myM; i++) {
			for (int mask = 0; mask < (1 << myN); mask++) {
				/**
				 *	We don't need any constraints (the mask) for sets of input variables, 
				 *	which are not possible.
				 */
				bool useless = true;
				for (int j = 0; j < myOuts; j++) {
					if (myValues[j][mask] != 2) {
						useless = false;
						break;
					}
				}
				if (useless) {
					continue;
				}
				for (int j1 = 0; j1 < i; j1++) {
					for (int j2 = 0; j2 < i; j2++) {
						for (int i1 = 0; i1 < 2; i1++) {
							for (int i2 = 0; i2 < 2; i2++) {
								for (int k = 0; k < 2; k++) {
									Clause *c = new Clause();
									c->addLiteral(Variable(1, get3List(i, 0, j1)), false);
									c->addLiteral(Variable(1, get3List(i, 1, j2)), false);
									/**
									 * Variables of type 2 are true iff
									 * j1-gate has value 1 when given
									 * assingment (mask) is on circuit input.
									 */
									c->addLiteral(Variable(2, get2List(j1, mask)), i1 == 0);
									c->addLiteral(Variable(2, get2List(j2, mask)), i2 == 0);
									/**
									 * Variables of type 0 are true iff i-gate
									 * has value 1 when boolean values i1 and i2 are its 
									 * input.
									 */
									c->addLiteral(Variable(0, get3List(i, i1, i2)), k == 0);
									c->addLiteral(Variable(2, get2List(i, mask)), k == 1);
									/**
									 * This clause means that if j1 and
									 * j2-gates are inputs of i-gate and j1
									 * and j2-gates have some fixed values
									 * under current assignment, then i-gate
									 * also has some fixed value under mask.
									 */
									myFormula.push_back(*c);
								}
							}
						}
					}
				}
			 }
		}

		for (int i = 0; i < (1 << myN); i++) {
			for (int j = 0; j < myN; j++) {
				Clause *c = new Clause();
				c->addLiteral(Variable(2, get2List(j, i)), (i & (1 << j)) != 0);
				/**
				 * The base of construction: if we have some assignment to
				 * circuit, then it is an assignment to circuit inputs.
				 */
				myFormula.push_back(*c);
			}
		}
	
		for (int i = myN; i < myN + myM; i++) {
			/**
			 * The unused 6 functions of 16 boolean functions of 2 variables.
			 * They are actially not functions of 2 variables.
			 */
	  		for (int j = 0; j < unused_size; j++) {
	  			Clause *c = new Clause();
	  			for (int k = 0; k < 4; k++) {
	  				c->addLiteral(Variable(0, get3List(i, k / 2, k % 2)), (unused[j] & (1 << k)) == 0);
	  			}
				/**
				 * The clause encode that we don't need some boolean functions of 2 
				 * variables in constructions of our circuit.
				 */
				myFormula.push_back(*c);
			}
		}

		for (int k = 0; k < myOuts; k++) {
			Clause *c = new Clause();
			for (int i = myN; i < myN + myM; i++) {
				//
				// Variables of type 3 means that i-gate is k-output of the
				// circuit.
				//
				c->addLiteral(Variable(3, get2List(i,k)), true);
			}
			//	
			// Encode that k-th outpus is somewhere.
			// 			
			myFormula.push_back(*c);
			for (int j1 = myN; j1 < myN + myM; j1++) {
				for (int j2 = myN; j2 < j1; j2++) {
					c = new Clause();
					c->addLiteral(Variable(3, get2List(j1, k)), false);
					c->addLiteral(Variable(3, get2List(j2, k)), false);
					//
					// Encode that two gates cannot be k-th output together.
					//
					myFormula.push_back(*c);
				}
			}
			for (int i = 0; i < (1 << myN); i++) {
				if (myValues[k][i] == 2) {
					continue;
				}
				for (int j = myN; j < myN + myM; j++) {
					Clause *c = new Clause();
					c->addLiteral(Variable(3, get2List(j, k)), false);
					c->addLiteral(Variable(2, get2List(j, i)), myValues[k][i] == 1);
					//	
					// Encode that our circuit calculates that we want.
					//
					myFormula.push_back(*c);
				}
			}
		}
		
		for (int i = 0; i < myN + myM; i++) {
			Clause *c = new Clause();
			for (int k = 0; k < myOuts; k++) {
				c->addLiteral(Variable(3, get2List(i,k)), true);
			}
			
			for (int k = 0; k < 2; k++) {
				for (int j = i + 1; j < myN + myM; j++) {
					c->addLiteral(Variable(1, get3List(j, k, i)), true);
				}
			}
			/**
			 * The clause encodes that the i-gate should be input for one of the 
			 * next gates or output if the circuit.
			 */ 
			myFormula.push_back(*c);
		}
//		addBottleneck();
//		addNonKillWithOneAssignment();
//		addSomeStrangeCondition();


		ofstream vout;
		vout.open("sat.vars");
		for (unsigned int i = 0; i < myVariablesList.size(); i++) {
			vout << (i + 1) << " " << VarToString(myVariablesList[i]) << endl;
		}
		vout.close();
	}

	string toString() {
		ostringstream result;
		result << "p cnf " << myVariables.size() << " " << myFormula.size() << endl;
		result << toSolverString();
		return result.str();
	}

	string toSolverString() {
		ostringstream result;
		for (unsigned int i = 0; i < myFormula.size(); i++) {
			result << myFormula[i].toSolverString();
		}
		return result.str();
	}

	vector<int> get2List(int v1, int v2) {
		vector<int> list;
		list.push_back(v1);
		list.push_back(v2);
		return list;
	}
	
	vector<int> get3List(int v1, int v2, int v3) {
		vector<int> list;
		list.push_back(v1);
		list.push_back(v2);
		list.push_back(v3);
		return list;
	}
};
#endif
