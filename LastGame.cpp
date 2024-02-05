#include <SDL.h> 
#include <iostream>
#include <math.h>
#include <vector>
using namespace std;
int w = 1920;
int h = 1080;
int ca = 64;
int w2 = w / ca;
int h2 = h / ca;
SDL_Window* window = NULL;
SDL_Surface* surface = NULL;
SDL_Event event;
#define mod(x, N) (x < 0) ? (x % N + N) : (x % N)
void line(SDL_Surface* surface, float x, float y, float x2, float y2, Uint32 &color)
{
	if (x < 0 or y < 0 or x2 < 0 or y2 < 0) return;
	float dx = x2 - x;
	float dy = y2 - y;
	static Uint32* pixels = (Uint32*)surface->pixels;
	static auto w = surface->w;
	if (abs(dx) > abs(dy))
	{
		if (x2 - x < 0) { swap(x, x2); swap(y, y2); }
		for (dy /= dx; x < x2; x++, y += dy)
			*(pixels + (short)(y) * w + (short)x) = color;
		return;
	}
	if (y2 - y < 0) { swap(x, x2); swap(y, y2); }
	for (dx /= dy; y < y2; y++, x += dx)
		*(pixels + (short)(y)*w + (short)x) = color;
	return;
}


/*Алгоритм: 
1. создание начальной матрицы для работы с чанками (пока подобие чанков);
2. Инициализация каждого нода рандомными кординатами относительно чанка;
3. Возможно задание каждому узлу +приоритета влияющего на длинну в зависимости от расположения (чанк с водой, горами, ресурсами и так. далее;
4. Создание вектора дорог и закидывание в него всех существующих вариантов дорог
   (каждый раз связываясь с другим узлом, проверяем есть ли связь в той стороне, если нет создаём и выссчитываем);
5. Каждый узел выссчитывает максимальную длинну дороги (с учётом приоритетов) с одной из сторон, высчитываем для каждой точки с временным temp (номер узла, длинна),
   берём индекс на 0 и длинну первого узла как начальную, ищем минимальную, далее ищем минимальную из всех остальных узлов и больше другого,
   в итоге ставим статусы до нашего лучшего варианта дорогам, каждой дороге ставим +1 за соответствие;
6. Далее проверяем дорогу на наличие 2 узлов, если есть добовляем, нет убираем, остальные дороги в массиве нужно контролировать, 
   скажем так мы будем использовать другой вектор, так как количество перезаписей слишком велико, то подчищяем все узлы, тем более что на один узел приходится 3 дороги постоянно,
   теперь при высчитывании используем либо временный либо новый массив, на углах  можно например изменить статус на обработан или нет у каждой дороги, 
   это статус именно дороги, таким образом мы работаем, однако создаём индекс таблицу, показывающую последние удалённые элементы чтобы вместо них подставить новые;
7. Далее смотрим на высчитанные дороги, считаем по ним возможные узлы и тому подобное, но может каждому узлу нужна одна большая дорога минимум, или будет зацикливание...*/
class Road;
class Node
{
public:
	int x, y;
	Road** roads;
	short status;
	short l;
	int chunk_x, chunk_y;
};
class Road
{
public:
	Node *a, *b;
	short l;
	short status;
};
class Matrix
{
public:
	short matrix_size;
	short matrix_size05;
	short chunk_size;
	Node*** nodes;
	vector <Road*> roads;
	int seed;
	class Player
	{
		int chunk_x, chunk_y;
	public:
		Player(int x, int y, int chunk_size)
		{
			chunk_x = x / chunk_size;
			chunk_x = x / chunk_size;
		}
	};
	Player* player;
	Matrix(short matrix_size, short chunk_size) : matrix_size(matrix_size + 2), chunk_size(chunk_size)
	{
		srand(time(0));
		seed = rand();
		player = new Player(0, 0, chunk_size);
		nodes = new Node**[this->matrix_size];
		for (short i = 0; i < this->matrix_size; i++)
			{
			nodes[i] = new Node*[this->matrix_size];
			for (short j = 0; j < this->matrix_size; j++)
				{
					nodes[i][j] = new Node;
					nodes[i][j]->roads = new Road * [8];
					for (short k = 0; k < 8; k++)
					{
						nodes[i][j]->roads[k] = nullptr;
					}
				}
			}			
	}
	short myrand(int a)
	{
		a = (a ^ 61) ^ (a >> 16);
		a += (a << 3);
		a ^= (a >> 4);
		a *= seed; //0x27d4eb2d;
		a ^=  (a >> 15);
		return a%chunk_size;
	}
	Node* node(int x, int y)
	{
		Node* temp = nodes[mod(x, matrix_size)][mod(y, matrix_size)];
	}
	void init_node()
	{
		short i_chunk = 0;
		for (short i = 0; i < matrix_size; i++, i_chunk+= chunk_size)
		{
			short j_chunk = 0;
			for (short j = 0; j < matrix_size; j++, j_chunk += chunk_size)
			{
				//nodes[i][j]->x = rand() % chunk_size + i_chunk;
				//nodes[i][j]->y = rand() % chunk_size + j_chunk;;
				nodes[i][j]->x = myrand(5*i << 8 | 3*j) + i_chunk;
				nodes[i][j]->y = myrand(7*j << 8 | 2*i) + j_chunk;
			}
		}
	}
	void one_road(Node* node2, int num, Node* node)
	{
		
		if (!node->roads[num])
		{
			Road* temp = new Road;
			*temp = { node, node2, (short)sqrt(pow(node->x - node2->x, 2) + pow(node->y - node2->y, 2)), 0 };
			node2->roads[7 - num] = temp;
			node->roads[num] = temp;
			roads.push_back(temp);
		}
	}
	void all_roads_init()
	{
		for (int i = 1; i < matrix_size-1; i++)
		{
			for (int j = 1; j < matrix_size-1; j++)
			{
				auto node = nodes[i][j];
				one_road(nodes[i - 1][j - 1], 0, node);
				one_road(nodes[i][j - 1], 1, node);
				one_road(nodes[i - 1][j], 2, node);
				one_road(nodes[i + 1][j - 1], 3, node);
				one_road(nodes[i - 1][j + 1], 4, node);
				one_road(nodes[i + 1][j], 5, node);
				one_road(nodes[i][j + 1], 6, node);
				one_road(nodes[i + 1][j + 1], 7, node);
			}
		}
	}
	void all_roads_reinit()
	{
		for (auto& road : roads)
		{
			road->status = 0;
			road->l = (short)sqrt(pow(road->a->x - road->b->x, 2) + pow(road->a->y - road->b->y, 2));
		}
	}
	inline void all_node_status()
	{
		for (int i = 1; i < matrix_size - 1; i++)
		{
			for (int j = 1; j < matrix_size - 1; j++)
			{
				auto node = nodes[i][j];
				node->l = 0;
				for (int i = 0; i < 8; ++i)
				{
					if (node->roads[i]->status > 0)
						node->l += node->roads[i]->l;
				}
				if (node->l < matrix_size)
					for (int i = 0; i < 8; i++)
					{
						if (node->roads[i]->status > 0)
							node->roads[i]->status = 4;
					}
			}
		}
	}
	void all_roads_status()
	{
		int max_l = chunk_size * sqrt(2);
		for (auto& road : roads)
		{	
			//if (road->status != 0) continue;
			if (road->l < max_l / 4)
				road->status += 4;
			else if (road->l < max_l / 3)
				road->status += 3;
			else if (road->l < max_l / 2)
				road->status += 2;
			else if (road->l < max_l)
				road->status += 1;
		}
	}
};


int main(int argc, char* args[])
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) 
	printf("SDL не смог запуститься! SDL_Error: %s\n", SDL_GetError());
	else
	{
		window = SDL_CreateWindow("Урок1", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
		if (window == NULL)
		printf("Окно не может быть создано! SDL_Error: %s\n", SDL_GetError());
	}
	surface = SDL_GetWindowSurface(window);
	Uint32 white = SDL_MapRGBA(surface->format, 255, 255, 255, 255);
	Uint32 red = SDL_MapRGBA(surface->format, 255, 0, 0, 255);
	Uint32 blue = SDL_MapRGBA(surface->format, 0, 255, 0, 255);
	Uint32 green = SDL_MapRGBA(surface->format, 0, 0, 255, 255);
	Uint32 black = SDL_MapRGBA(surface->format, 0, 0, 0, 255);
	Matrix matrix(64, 16);
	matrix.init_node();
	matrix.all_roads_init();
	matrix.all_roads_status();
	int counter = 0;
	while (true)
	{
		SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0x00, 0x00, 0x00));
		const Uint8* keyboard_state_array = SDL_GetKeyboardState(NULL);

		SDL_PollEvent(&event);
		if (keyboard_state_array[SDL_SCANCODE_ESCAPE])
		{
			SDL_DestroyWindow(window);
			SDL_Quit();
			return 0;
		}
		SDL_FillRect(surface, NULL, black);
		int lines = 0;
		for (auto& road : matrix.roads)
		{
			if (road->status == 1)
			{
				line(surface, road->a->x, road->a->y, road->b->x, road->b->y, white);
				lines++;
			}
			else if (road->status == 2)
			{
				line(surface, road->a->x, road->a->y, road->b->x, road->b->y, red);
				lines++;
			}
			else if (road->status == 3)
			{
				line(surface, road->a->x, road->a->y, road->b->x, road->b->y, blue);
				lines++;
			}
			else if (road->status == 4)
			{
				line(surface, road->a->x, road->a->y, road->b->x, road->b->y, green);
				lines++;
			}
		}
		counter++;
		std::cout << counter << ' ' << lines << std::endl;
		SDL_UpdateWindowSurface(window);
		matrix.init_node();
		matrix.all_roads_reinit();
		matrix.all_roads_status();
	}
}
