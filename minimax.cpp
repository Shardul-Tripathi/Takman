#include "gamestate.h"
#include "getmoves.h"
#include "minimax.h"

// #define neg	std::numeric_limits<int>::min()
// #define pos	std::numeric_limits<int>::max()
#define neg -100000
#define pos 100000
#define ttsize 102471123
#define hsize 1123143

#define fc 100
#define cs 75
#define w 29
#define c 10
#define ccap 12
#define s 15
#define inf 15

using namespace std;

int height;
int i_ngb, j_ngb, h_ngb;
int i_lcrt,i_rcrt;
int j_lcrt,j_rcrt;
int peice, opp_peice;

vector<pair<int,int> > neighbours;	///////////////////////////////
vector<string> d_moves;
int dmoves_size;

int nodes = 0;
int flatcount = 0;
int centre_control = 0;
int stack_color = 0;
int stack = 0;
int total_influence = 0;
int mycolor=0; int oppcolor=0;
int row = 0; int col = 0;
int row_influence = 0; int col_influence = 0;

int influence_table[5][5];	///////////////////////////////////

int infl[7][7] =
    {
//    	  0  1  2  3  -1  -2  -3
         {0, 0, 0, 0,  0,  0, 0},
         {1, 2, 0, 0,  1,  0, 0},
         {1, 1, 0, 0,  1,  0, 0},
         {1, 1, 1, 0,  1,  1, 0},
         {-1,-1,0, 0, -2,  0, 0},
         {-1,-1,0, 0, -1,  0, 0},
         {-1,-1,-1,0, -1, -1, 0}
    };
    //0,1,2,3 -> 0,1,2,3
    //-1->4 , -2->5, -3->6
    //p<0?-p+3:p

struct evalInfo 
{
	entry_type type;
	int value;
	// int ubound;
	// int lbound;
	int depth;
	// int bm;
	string bmove;

	// evalInfo(int u, int l, int d, string bms) : ubound(u), lbound(l), depth(d), bmove(bms) { }
	evalInfo(entry_type t, int v, int d, string bms) : type(t), value(v), depth(d), bmove(bms) { }
};

unordered_map<uint64_t,evalInfo> ttable(ttsize);
unordered_map<string,int> history;

int flatwin(gamestate* game)
{
	int myflats = 0;
	int otherflats = 0;
	int n = game->n;
	for (int i=0; i<n; i++)
		for (int j=0; j<n; j++)
		{
			if (game->board[i][j][game->height[i][j]-1]==1||game->board[i][j][game->height[i][j]-1]==3)
				myflats++;
			if (game->board[i][j][game->height[i][j]-1]==-1||game->board[i][j][game->height[i][j]-1]==-3)
				otherflats++;
		}
	return (myflats-otherflats)*pos/(myflats+otherflats); ///////// also return difference of leftover flatstones (used in case of draw)
}

int eval(gamestate* game)
{
	flatcount = 0;
	centre_control = 0;
	stack_color = 0;
	stack = 0;
	total_influence = 0;
	mycolor=0;  oppcolor=0;

	int n = game->n;
	for (int i=0; i<n; i++)
		for (int j=0; j<n; j++)
			influence_table[i][j]=0;
	

	for (int i=0; i<n; i++)
	{
		for (int j=0; j<n; j++)
		{
			height = game->height[i][j];
			if (height==0) continue;
			peice = game->board[i][j][height-1];
			switch(peice)
			{
				case 1: flatcount += fc; if (i>0 && i<n-1 && j>0 && j<n-1) centre_control += c; break;
				case -1: flatcount -= fc; if (i>0 && i<n-1 && j>0 && j<n-1) centre_control -= c; break;
				case 2: flatcount += w; break;
				case -2: flatcount -= w; break;
				case 3: flatcount += cs; if (i>0 && i<n-1 && j>0 && j<n-1) centre_control += ccap; break;
				case -3: flatcount -= cs; if (i>0 && i<n-1 && j>0 && j<n-1) centre_control -= ccap; break;
				default: break;
			}

			if (height > 1) //// stack
			{
				stack = 0;
				mycolor = 0;
				oppcolor = 0;
				for (int h=0; h<height-1; h++)
				{
					if (game->board[i][j][h] == 1)
						mycolor++;
					else
						oppcolor++;
				}
				stack = mycolor*s - oppcolor*s;
				
				if (peice==1) stack += (stack>0)?2*s:s;
				else if (peice==-1) stack -= (stack>0)?0.5*s:2*s;
				else if (peice==3) stack += (stack>0)?s*5:2*s;
				else if (peice==-3) stack -= (stack>0)?s*2:s*5;
				else if (peice==2) stack += (stack>0)?3*s:s;
				else stack -= (stack>0)?s:3*s;
				stack_color += stack;

				if (mycolor>0 && peice > 0)
				{
					neighbours.clear();
					game->getNeighbours(i,j,mycolor+1,neighbours);
					i_lcrt = -1;
					i_rcrt = n;
					j_lcrt = -1;
					j_rcrt = n;
					for (int k=0; k<neighbours.size(); k++)
					{
						i_ngb = neighbours[k].first;
						j_ngb = neighbours[k].second;
						if (!((i_ngb>i_lcrt && i_ngb<i_rcrt)&&(j_ngb>j_lcrt && j_ngb<j_rcrt))) continue;
						opp_peice = game->board[i_ngb][j_ngb][game->height[i_ngb][j_ngb]-1];
						if (peice==1 || peice==2)
						{
							if (opp_peice!=-3 && opp_peice!=-2)
								influence_table[i_ngb][j_ngb]++;
							else
							{
								influence_table[i_ngb][j_ngb]-=2;
								(i_ngb<i ? i_lcrt:i_rcrt) = i_ngb;
								(j_ngb<i ? j_lcrt:j_rcrt) = j_ngb;
							}
						}
						else
						{
							if (opp_peice==-3)
							{
								influence_table[i_ngb][j_ngb]-=2;
								(i_ngb<i ? i_lcrt:i_rcrt) = i_ngb;
								(j_ngb<i ? j_lcrt:j_rcrt) = j_ngb;
							}
							else if(opp_peice==-2)
							{
								if (game->board[i][j][height-2]<0)
								{
									influence_table[i_ngb][j_ngb]-=2;
									(i_ngb<i ? i_lcrt:i_rcrt) = i_ngb;
									(j_ngb<i ? j_lcrt:j_rcrt) = j_ngb;		
								}
								else
								{
									influence_table[i_ngb][j_ngb]++;
									(i_ngb<i ? i_lcrt:i_rcrt) = i_ngb;
									(j_ngb<i ? j_lcrt:j_rcrt) = j_ngb;
								}
							}
							else
								influence_table[i_ngb][j_ngb]++;
						}
					}
					
				}
				if (oppcolor>0 && peice < 0)
				{
					neighbours.clear();
					game->getNeighbours(i,j,oppcolor+1,neighbours);
					i_lcrt = -1;
					i_rcrt = n;
					j_lcrt = -1;
					j_rcrt = n;
					for (int k=0; k<neighbours.size(); k++)
					{
						i_ngb = neighbours[k].first;
						j_ngb = neighbours[k].second;
						if (!((i_ngb>i_lcrt && i_ngb<i_rcrt)&&(j_ngb>j_lcrt && j_ngb<j_rcrt))) continue;
						opp_peice = game->board[i_ngb][j_ngb][game->height[i_ngb][j_ngb]-1];
						if (peice==-1 || peice==-2)
						{
							if (opp_peice!=3 && opp_peice!=2)
								influence_table[i_ngb][j_ngb]--;
							else
							{
								influence_table[i_ngb][j_ngb]+=2;
								(i_ngb<i ? i_lcrt:i_rcrt) = i_ngb;
								(j_ngb<i ? j_lcrt:j_rcrt) = j_ngb;
							}
						}
						else
						{
							if (opp_peice==3)
							{
								influence_table[i_ngb][j_ngb]+=2;
								(i_ngb<i ? i_lcrt:i_rcrt) = i_ngb;
								(j_ngb<i ? j_lcrt:j_rcrt) = j_ngb;
							}
							else if(opp_peice==2)
							{
								if (game->board[i][j][height-2]>0)
								{
									influence_table[i_ngb][j_ngb]+=2;
									(i_ngb<i ? i_lcrt:i_rcrt) = i_ngb;
									(j_ngb<i ? j_lcrt:j_rcrt) = j_ngb;		
								}
								else
								{
									influence_table[i_ngb][j_ngb]--;
									(i_ngb<i ? i_lcrt:i_rcrt) = i_ngb;
									(j_ngb<i ? j_lcrt:j_rcrt) = j_ngb;
								}
							}
							else
								influence_table[i_ngb][j_ngb]--;
						}
					}
					
				}

			}
			if (height==1)
			{
				neighbours.clear();
				peice = game->board[i][j][0];
				game->getNeighbours(i,j,1,neighbours);
				for (int k=0; k<neighbours.size(); k++)
				{
					i_ngb = neighbours[k].first;
					j_ngb = neighbours[k].second;
					h_ngb = game->height[i_ngb][j_ngb];
					opp_peice = h_ngb>0?game->board[i_ngb][j_ngb][h_ngb-1]:0;

					influence_table[i_ngb][j_ngb] += infl[peice<0?-peice+3:peice][opp_peice<0?-opp_peice+3:opp_peice];
				}			
			}
		}
	}

	row = 0; col = 0;
	row_influence = 0; col_influence = 0;

	for (int i=0; i<n; i++)
	{
		row = 0;
		for (int j=0; j<n; j++)
		{
			total_influence += influence_table[i][j]*inf;
			row += influence_table[i][j];
		}
		if (row>=2*n)
			row_influence += 2*inf*row;
		if (row<=-2*n)
			row_influence -= 2*inf*row;
	}
	for (int j=0; j<n; j++)
	{
		col = 0;
		for (int i=0; i<n; i++)
		{
			col += influence_table[i][j];;
		}
		if (col>=2*n)
			col_influence += 2*inf*col;
		if (col<=-2*n)
			col_influence -= 2*inf*col;
	}

	return (8*flatcount + centre_control + 2*stack_color + 2*total_influence + 7*row_influence/2 + 7*col_influence/2);
	// return (8*flatcount + centre_control + 3*stack_color)/10;
}

inline int max_val(int a, int b)
{
	return (a>b)?a:b;
}

inline int min_val(int a, int b)
{
	return (a>b)?b:a;
}

bool myComparison(const pair<string,int> &a,const pair<string,int> &b)
{
	return a.second>b.second;
	// if (a.second > b.second)
	// 	return true;
	// else if (a.second == b.second && b.first[0]=='S')
	// 	return true;
	// else return false;
}
bool myComparison2(const pair<string,int> &a,const pair<string,int> &b)
{
	return a.second<b.second;
}


void storeEntry(uint64_t hash, int value, entry_type type, int depth, string bmove)
{
	auto entry = ttable.find(hash);
	if (entry != ttable.end())
	{
		evalInfo* info = &(entry->second);
		info->type = type;
		info->value = value;
		info->depth = depth;
		info->bmove = bmove;
	}
	else
	{
		ttable.insert(make_pair(hash,evalInfo(type,value,depth,bmove)));
	}
}

// void storeEntry(uint64_t hash, int upper, int lower, int depth, string bmove)
// {
// 	auto entry = ttable.find(hash);
// 	if (entry != ttable.end())
// 	{
// 		evalInfo* info = &(entry->second);
// 		info->ubound = upper;
// 		info->lbound = lower;
// 		info->depth = depth;
// 		// info->bm = bm;
// 		info->bmove = bmove;
// 	}
// 	else
// 	{
// 		ttable.insert(make_pair(hash,evalInfo(upper,lower,depth,bmove)));
// 		// ttable[hash] = evalInfo(upper,lower,depth,bm,bmove);
// 	}
// }

// void storeUpper(uint64_t hash, int upper, int lower, int depth, string bmove)
// {
// 	auto entry = ttable.find(hash);
// 	if (entry != ttable.end())
// 	{
// 		evalInfo* info = &(entry->second);
// 		info->ubound = upper;
// 		info->depth = depth;
// 		// info->bm = bm;
// 		info->bmove = bmove;
// 	}
// 	else
// 	{
// 		ttable.insert(make_pair(hash,evalInfo(upper,lower,depth,bmove)));
// 		// ttable[hash] = evalInfo(upper,lower,depth,bm,bmove);
// 	}
// }

// void storeLower(uint64_t hash, int upper, int lower, int depth, string bmove)
// {
// 	auto entry = ttable.find(hash);
// 	if (entry != ttable.end())
// 	{
// 		evalInfo* info = &(entry->second);
// 		info->lbound = lower;
// 		info->depth = depth;
// 		// info->bm = bm;
// 		info->bmove = bmove;
// 	}
// 	else
// 	{
// 		ttable.insert(make_pair(hash,evalInfo(upper,lower,depth,bmove)));
// 		// ttable[hash] = evalInfo(upper,lower,depth,bm,bmove);
// 	}
// }

int mtdf (gamestate* game, int f, int d)
{
	int g = f;
	int beta;
	int u = pos;
	int l = neg;
	while (l < u)
	{
		if (g == l) beta = g + 1;
		else beta = g;
		// g = abtt(game,beta-1,beta,d,true);
		g = negamax(game,beta-1,beta,d,true);
		if (g < beta) 
			u = g;
		else
			l = g;
	}
	return g;
}

pair<int,string> ids(gamestate* game)
{
	ttable.clear();
	history.clear();
	// nodes = 0;
	static int count = 0;
	static int dlimit = 6;
	// if (count<7) dlimit = 6; else dlimit = 7;
	int guess = 0;
	int tm;
	string move;

	for (int d=1; d<=dlimit; d++)
	{
		nodes = 0;
		cerr<<"depth: "<<d;
		// guess = mtdf(game,guess,d);
		// guess = abtt(game,neg,pos,d,true);
		auto start = std::chrono::system_clock::now();
		guess = negamax(game,neg,pos,d,true);
		auto end = std::chrono::system_clock::now();
		tm = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()/1000;
		move = (&(ttable.find(game->getHash())->second))->bmove;
		cerr<<" abttmove: "<<move;
		cerr<<" val: "<<guess/10;
		cerr<<" time: "<<tm;
		cerr<<" branching: "<<pow(nodes,1.0/(d+1))<<endl;
	}
	count++;
	return make_pair(guess,move);
}

int negamax(gamestate* game, int alpha, int beta, int depth, bool maxNode)
{
	nodes++;
	int alphaOrig = alpha;
	auto entry = ttable.find(game->getHash());
	evalInfo* info;
	string bestmove = "";

	if (entry != ttable.end())
	{
		info = &(entry->second);
		if (info->depth >= depth)
		{
			if (info->type == exact)
				return info->value;
			else if (info->type == lowerbound)
				alpha = std::max(alpha,info->value);
			else if (info->type == upperbound)
				beta = std::min(beta,info->value);
			if (alpha >= beta)
				return info->value;
		}
		bestmove = info->bmove;
	}
	int color = maxNode ? 1:-1;
	int game_over = game->over();
	if (game_over == 1)
		return color*pos;
	else if (game_over == -1)
		return color*neg;
	else if (game_over == 2 || game_over == 3)
		return color*flatwin(game);
	else if (depth == 0)
		return color*eval(game);
	
	auto player = maxNode ? game->player_id : game->other_player;
	int bestValue = neg;
	bool localW2F;
	vector<string> moves; generate_moves(moves,game,player);
	int moves_size = moves.size();
	int v=0;

	vector<pair<string,int> > sorted_moves;
		for (int i=0; i<moves_size; i++)
		{
			v = history[moves[i]];
			sorted_moves.push_back(make_pair(moves[i],v));
		}
	sort(sorted_moves.begin(),sorted_moves.end(),myComparison);

	// if (bestmove != "") /////best move first
	// {
	// 	game->update_board(bestmove,player);
	// 	localW2F = game->wallToFlat;

	// 	v = -negamax(game,-beta,-alpha,depth-1,!maxNode);
		
	// 	game->wallToFlat = localW2F;
	// 	game->undo_move(bestmove,player);

	// 	bestValue = std::max(bestValue,v);
	// 	alpha = std::max(alpha,v);
	// 	if (alpha >= beta)
	// 		goto hell;
	// }

	for (int i=0; i<moves_size; i++)
	{
		// if (sorted_moves[i].first == bestmove) continue;

		game->update_board(sorted_moves[i].first,player);
		localW2F = game->wallToFlat;

		v = -negamax(game,-beta,-alpha,depth-1,!maxNode);
		
		game->wallToFlat = localW2F;
		game->undo_move(sorted_moves[i].first,player);

		if (v > bestValue)
		{
			bestValue = v;
			bestmove = sorted_moves[i].first;
		}
		
		if (bestValue >= beta)
		{
			// bestmove = sorted_moves[i].first;
			// cerr<<i<<endl;
			break;
		}
		alpha = std::max(alpha,bestValue);
	}

	hell:
	history[bestmove] += pow(2,depth);
	// history[bestmove] += depth*depth;
	if (bestValue <= alphaOrig)
		storeEntry(game->getHash(),bestValue,upperbound,depth,bestmove);
	else if (bestValue >= beta)
		storeEntry(game->getHash(),bestValue,lowerbound,depth,bestmove);
	else
		storeEntry(game->getHash(),bestValue,exact,depth,bestmove);
	return bestValue;
}

// int pvs (gamestate* game, int alpha, int beta, int depth, bool maxNode)
// {

// }

// int abtt (gamestate* game, int alpha, int beta, int depth, bool maxNode)
// {
// 	int value;
// 	auto entry = ttable.find(game->getHash());
	
// 	string bmove = "";

// 	if (entry != ttable.end())
// 	{
// 		evalInfo* info = &(entry->second);

// 		if (info->depth >= depth)
// 		{
// 			if (info->lbound >= beta)
// 				return info->lbound;
// 			if (info->ubound <= alpha || info->ubound == info->lbound)
// 				return info->ubound;
// 			alpha = max_val(alpha,info->lbound);
// 			beta = min_val(beta,info->ubound);
// 		}
// 		else
// 		{
// 			info->ubound = pos;
// 			info->lbound = neg;
// 		}
// 			// bm = info->bm;
// 			bmove = info->bmove;
// 	}

// 	int game_over = game->over(); //// 4 => not over

// 	if (game_over == 1)
// 		value = pos;
// 	else if (game_over == -1)
// 		value = neg;
// 	else if (game_over == 2 || game_over == 3)
// 		value = flatwin(game);
// 	else if (depth == 0)
// 		value = eval(game);
// 	else
// 	{
// 		bool localW2F;
// 		// bool bmdone = false;
// 		if (maxNode)
// 		{
// 			value = neg;
// 			int a = alpha;
// 			vector<string> moves; generate_moves(moves,game,game->player_id);
// 			int moves_size = moves.size();
// 			int temp;
// 			if (bmove != "") /////best move first
// 			{
// 				game->update_board(bmove,game->player_id);
// 				localW2F = game->wallToFlat;

// 				temp = abtt(game,a,beta,depth-1,false);
				
// 				game->wallToFlat = localW2F;
// 				game->undo_move(bmove,game->player_id);

// 				if (temp > value)
// 				{
// 					value = temp;
// 				}
// 				if (value >= beta)
// 					goto hell;
// 				a = max_val(a,value);
// 			}
// 			// else
// 			// 	bm = 0;
// 			for (int i=0; i<moves_size; i++)
// 			{
// 				// if (bmdone && i == bm) continue;
// 				if (moves[i]==bmove) continue;
// 				game->update_board(moves[i],game->player_id);
// 				localW2F = game->wallToFlat;

// 				temp = abtt(game,a,beta,depth-1,false);
				
// 				game->wallToFlat = localW2F;
// 				game->undo_move(moves[i],game->player_id);

// 				if (temp > value)
// 				{
// 					value = temp;
// 					// bm = i;
// 					bmove = moves[i];
// 				}
// 				if (value >= beta)
// 					break;
// 				a = max_val(a,value);
// 			}
// 		}
// 		else
// 		{
// 			value = pos;
// 			int b = beta;
// 			vector<string> moves; generate_moves(moves,game,game->other_player);
// 			int moves_size = moves.size();
// 			int temp;
// 			if (bmove != "") /////best move first
// 			{
// 				// bmdone = true;
// 				game->update_board(bmove,game->other_player);
// 				localW2F = game->wallToFlat;

// 				temp = abtt(game,alpha,b,depth-1,true);
				
// 				game->wallToFlat = localW2F;
// 				game->undo_move(bmove,game->other_player);

// 				if (temp < value)
// 				{
// 					value = temp;
// 				}
				
// 				if (value <= alpha)
// 					goto hell;
// 				b = min_val(b,value);
// 			}
// 			// else
// 			// 	bm = 0;
// 			for (int i=0; i<moves_size; i++)
// 			{
// 				// if (bmdone && i == bm) continue;
// 				if (moves[i]==bmove) continue;
// 				game->update_board(moves[i],game->other_player);
// 				localW2F = game->wallToFlat;

// 				temp = abtt(game,alpha,b,depth-1,true);
				
// 				game->wallToFlat = localW2F;
// 				game->undo_move(moves[i],game->other_player);

// 				if (temp < value)
// 				{
// 					value = temp;
// 					// bm = i;
// 					bmove = moves[i];
// 				}
				
// 				if (value <= alpha)
// 					break;
// 				b = min_val(b,value);
// 			}
// 		}
// 	}
// 	// if (game_over == 4 && depth != 0)
// 	// {
		
// 	// }
// 	// void storeEntry(uint64_t hash, int upper, int lower, int depth, int bm, string bmove)
// 	hell:
// 	if (value <= alpha)
// 		storeUpper(game->getHash(),value,neg,depth,bmove);
// 	else if (value > alpha && value < beta)
// 		storeEntry(game->getHash(),value,value,depth,bmove);
// 	else
// 		storeLower(game->getHash(),pos,value,depth,bmove);
// 	return value;
// }



pair<int,string> value(gamestate* game, int cutoff, int depth, int alpha, int beta, bool maxNode)
{
	int game_over = game->over(); ///////// check who made the move previously

	if (game_over==1) ////// I won by road
		return make_pair(pos,"");

	if (game_over==-1) ////// Opp won by road
		return make_pair(neg,"");

	if (game_over==2) ////// Someone ran out of tiles
		return make_pair(flatwin(game),"");

	if (game_over==3) /////// Board full
		return make_pair(flatwin(game),"");

	if (cutoff==depth)
		return make_pair(eval(game),""); //// if it is maxnode =>
										//// came from a move of minnode => evaluate wrt to other player	

	bool localW2F;

	if(depth==0) 
	{
		d_moves.clear();
		generate_moves(d_moves,game,game->player_id);
		dmoves_size = d_moves.size();

		for(int i=0;i<dmoves_size;i++) 
		{
			game->update_board(d_moves[i],game->player_id);
			game_over = game->over();
			if(game_over==1) 
			{
				game->undo_move(d_moves[i],game->player_id);
				return make_pair(pos,d_moves[i]);
			}
			if((game_over==2 || game_over==3) && flatwin(game)>0) 
			{
				game->undo_move(d_moves[i],game->player_id);
				return make_pair(pos,d_moves[i]);
			}
			game->undo_move(d_moves[i],game->player_id);
		}
	}

	if (maxNode)
	{
		int max = neg;
		
		vector<string> moves; generate_moves(moves,game,game->player_id);
		
		int moves_size = moves.size();
		int max_index = 0;
		int temp;

		vector<pair<string,int> > sorted_moves;
		for (int i=0; i<moves_size; i++)
		{
			game->update_board(moves[i],game->player_id);
			localW2F = game->wallToFlat;
			temp = eval(game);
			game->wallToFlat = localW2F;
			game->undo_move(moves[i],game->player_id);
			sorted_moves.push_back(make_pair(moves[i],temp));
		}
		sort(sorted_moves.begin(),sorted_moves.end(),myComparison);

		for (int i=0; i<moves_size; i++)
		{
			game->update_board(sorted_moves[i].first,game->player_id);
			localW2F = game->wallToFlat;
			temp = value(game,cutoff,depth+1,alpha,beta,false).first;
			if (temp>max)
			{
				max = temp;
				max_index = i;
			}
			game->wallToFlat = localW2F;
			game->undo_move(sorted_moves[i].first,game->player_id);
			if (max >= beta)
				return make_pair(max,sorted_moves[i].first);
			alpha = max_val(alpha,max);
		}
		return make_pair(max,sorted_moves[max_index].first);
	}
	else
	{
		int min = pos;
		vector<string> moves; generate_moves(moves,game,game->other_player);
		int moves_size = moves.size();
		int min_index = 0;
		int temp;

		vector<pair<string,int> > sorted_moves;
		for (int i=0; i<moves_size; i++)
		{
			game->update_board(moves[i],game->other_player);
			localW2F = game->wallToFlat;
			temp = eval(game);
			game->wallToFlat = localW2F;
			game->undo_move(moves[i],game->other_player);
			sorted_moves.push_back(make_pair(moves[i],temp));
		}
		sort(sorted_moves.begin(),sorted_moves.end(),myComparison2);

		for (int i=0; i<moves_size; i++)
		{
			game->update_board(sorted_moves[i].first,game->other_player);
			localW2F = game->wallToFlat;
			temp = value(game,cutoff,depth+1,alpha,beta,true).first;
			if (temp<min)
			{
				min = temp;
				min_index = i;
			}
			game->wallToFlat = localW2F;
			game->undo_move(sorted_moves[i].first,game->other_player);
			if (min <= alpha) 
				return make_pair(min,sorted_moves[i].first);
			beta = min_val(beta,min);
		}
		return make_pair(min,sorted_moves[min_index].first);
	}
	
}