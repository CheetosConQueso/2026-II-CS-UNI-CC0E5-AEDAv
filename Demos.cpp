
#include <iostream>
#include <fstream> // ofstream para escribir en archivo
#include <string>
#include <thread>
#include <vector>
#include "foreach.h"
#include "containers/Container.h"
#include "containers/ContainerIO.h"
#include "containers/vector.h"
#include "containers/LinkedList.h"
#include "containers/CircularLinkedList.h"
#include "containers/DoublyLinkedList.h"
#include "containers/CircularDoublyLinkedList.h"
#include "containers/Heap.h"
#include "containers/BinaryTree.h"
#include "Demos.h"
using namespace std;

void AddOne(GeneralNode<TX> &node) {
    node.getValue() += 1;
}

template <typename T>
void AddX(GeneralNode<T> &node, T x) {
    node.getValue() += x;
}

void Square(GeneralNode<TX> &node) {
    node.getValue() *= node.getValue();
}

// Predicado generico para FirstThat/call: true si el valor del nodo es
// mayor que 'threshold'. Generico en Node y T para servir tanto a
// Vector<VectorAscTraits<TX>> (int) como a Vector<VectorAscTraits<string>>.
template <typename Node, typename T>
bool IsGreaterThan(Node &node, T threshold) {
    return node.getValue() > threshold;
}

template <typename Node>
void PrintNode(Node &node, ostream &os) {
    os << node << " ";
}
const int NThreads = 5;

// Inserta secuencialmente cada pareja (valor, ref) de 'values' en el
// Container, una por una. La insercion concurrente queda aislada en
// DemoRaceCondition(), que es donde se estudia esa problematica.
template <typename Container>
void InsertElements(Container &container,
                     const vector<pair<typename Container::value_type, Ref>> &values) {
    for (const auto &v : values)
        container.push_back(v.first, v.second);
}

template <typename Container>
void TestContainer(Container &container,
                    const vector<pair<typename Container::value_type, Ref>> &values,
                    const string &filename) {
    InsertElements(container, values);

    // Impresion usando write()
    cout << "Container using write():\n\t";
    container.write(cout);
    cout << endl;

    // Escritura hacia un archivo, en modo append para acumular cada estado
    // (el archivo se deja vacio una vez al inicio, ver DemoVector)
    ofstream of(filename, ios::app);
    container.write(of);
    of << endl;
    of.close();

    // Escritura en pantalla usando cout directamente (operator<<)
    cout << "Container using cout directly:\n\t";
    cout << container << endl;
}

// Prueba los recorridos del Container hacia adelante (begin/end) y hacia
// atras (rbegin/rend) solamente, imprimiendo los elementos en cada sentido.
template <typename Container>
void TestTraversal(Container &container) {
    using Node          = typename Container::Node;
    using value_type    = typename Container::value_type;

    cout << "Forward traversal:\n\t[";
    container.call(PrintNode<Node>, cout);
    cout << "]" << endl;

    cout << "Backward traversal:\n\t[";
    container.rcall(PrintNode<Node>, cout);
    cout << "]" << endl;

    // Prueba de call()/FirstThat() con valor de retorno: busca el primer
    // nodo cuyo valor sea mayor que el del primer elemento del container.
    if (!container.empty()) {
        value_type threshold = container.begin()->getValue();

        try {
            Node& found = container.FirstThat(IsGreaterThan<Node, value_type>, threshold);

            cout << "FirstThat (primer valor > " << threshold
                << "):\n\t" << found << endl;

        } catch (const std::exception& e) {
            cerr << "FirstThat: " << e.what() << endl;
        }
    }
}

// Prueba los recorridos hacia únicamente hacia adelante, especialmente para una LinkedList
template <typename Container>
void TestForwardTraversal(Container &container){
    using Node = typename Container::Node;

    cout << "Prueba de función call con std::cout para el contenedor:\n\t[";
    container.call(PrintNode<Node>, cout);
    cout << "]" << endl;

    cout << "Prueba de uso de iteradores para impresión del contenedor:\n\t[";
    for (auto &&node : container)
        cout << node << " ";
    cout << "]" << endl;

    // Prueba de FirstThat(): primer nodo cuyo valor sea mayor que el primero
    Node threshold{};
    bool first = true;

    const Node& found = container.cFirstThat([&](const Node& node) {
        if (first) {
            threshold = node;
            first = false;
            return false;
        }

        return node.getValue() > threshold.getValue();
    });
    cout << "FirstThat (primer valor > " << threshold << "):\n\t" << found << endl;

}

void DemoVector() {
    // Dejamos los archivos vacios para que TestContainer acumule (append)
    // el estado del container tras cada paso
    ofstream("vector.txt", ios::trunc).close();

    // Cada elemento es una pareja (valor, ref) que se guarda en un Node;
    // el constructor initializer_list arma el Vector inicial de una vez
    Vector<VectorAscTraits<TX>> vec({{0, 10}, {1, 11}, {2, 12}, {3, 13}, {4, 14}});
    TestContainer(vec, {{5, 15}, {6, 16}, {7, 17}, {8, 18}, {9, 19}}, "vector.txt");
    TestTraversal(vec);

    ofstream("vector_str.txt", ios::trunc).close();
    Vector<VectorAscTraits<string>> strVec;
    TestContainer(strVec, {{"Hello", 1}, {"World", 2}}, "vector_str.txt");
    TestTraversal(strVec);
}

// Insertamos muchos elementos (generados en un loop, no a mano)
// concurrentemente y comparamos cuantos deberian haber entrado contra
// cuantos entraron realmente. Si push_back no estuviera sincronizado
// (sin el mutex/lock_guard actual) esto perderia inserciones o crashearia;
// con el mutex protegiendo push_back/resize, deberia dar siempre 0 perdidas.
void DemoRaceCondition() {
    const size_t N = 200000;

    vector<pair<TX, Ref>> values(N);
    for (size_t i = 0; i < N; ++i)
        values[i] = {static_cast<TX>(i), static_cast<Ref>(i)};

    Vector<VectorAscTraits<TX>> vec;

    // Insercion concurrente: NThreads workers insertando en paralelo sobre
    // el mismo Vector, cada uno con un subconjunto entrelazado (stride)
    size_t n = values.size();
    vector<thread> workers;
    for (int t = 0; t < NThreads; ++t) {
        workers.emplace_back([&vec, &values, n, t](){
            for (size_t i = t; i < n; i += NThreads)
                vec.push_back(values[i].first, values[i].second);
        });
    }
    for (auto &worker : workers)
        worker.join();

    long long expectedSum = 0;
    for (auto &v : values) expectedSum += v.first;

    long long actualSum = 0;
    for (size_t i = 0; i < vec.size(); ++i) actualSum += vec[i].getValue();

    cout << "DemoRaceCondition: se esperaban " << N << " elementos, "
         << "el Vector quedo con " << vec.size() << endl;
    cout << "  suma esperada = " << expectedSum
         << ", suma obtenida = " << actualSum << endl;

    if (vec.size() != N || actualSum != expectedSum)
        cout << "  *** Race condition detectada: se perdieron inserciones (push_back / resize sin sincronizar) ***" << endl;
    else
        cout << "  No se perdio ningun elemento: el mutex de push_back/resize "
             << "sincroniza correctamente las inserciones concurrentes" << endl;
}

// TODO: Implementar DemoLinkedList() para probar la lista enlazada y sus iteradores.
void DemoLinkedList()
{
    using IntLinkedList = LinkedList<LinkedListAscTraits<TX>>;

    ofstream("linkedlist.txt", ios::trunc).close();

    // Pruebas de pushback
    cout <<"--PRUEBAS DE PUSHBACK--\n";
    IntLinkedList list;
    TestContainer(list, {{5, 15}, {1, 11}, {8, 18}, {3, 13}}, "linkedlist.txt");
    TestForwardTraversal(list);

    auto elements = std::vector<pair<TX, Ref>>({{0, 10}, {1, 11}, {2, 12}, {3, 13}, {4, 14}});
    for(auto& e : elements)
        list.push_back(e.first, e.second);

    // Pruebas de insert
    cout <<"-- PRUEBAS DE INSERT --\n";
    list.insert(4, 12);
    list.insert(11, 20);
    list.insert(7, 6);
    cout << "LinkedList luego de hacer 3 inserts:\n\t" << list << endl;

    // Prueba de apply function
    cout <<"-- PRUEBA DE APPLY FUNCTION --\n";
    list.ApplyFunction(AddOne);
    cout << "LinkedList tras usar Add One a sus elementos:\n\t" << list << endl;
    list.ApplyFunction(AddX<TX>, TX(10));
    cout << "LinkedList tras usar AddX(10) a sus elementos:\n\t" << list << endl;

    // Prueba de clear
    cout <<"-- PRUEBAS DE CLEAR --\n";
    list.clear();
    cout << "LinkedList luego de usar clear:\n\t" << list << endl;
    list.push_back(10, 2);
    cout << "LinkedList luego de un nuevo push_back:\n\t" << list << endl;

    // Prueba de lectura
    cout <<"-- PRUEBA DE LECTURA --\n";
    IntLinkedList new_list;
    ifstream in("linkedlist.txt");
    in >> new_list;
    in.close();
    cout << "LinkedList leida desde archivo:\n\t" << new_list << endl;
    TestForwardTraversal(new_list);
}

void DemoCircularLinkedList()
{
    using IntCircularLinkedList = CircularLinkedList<LinkedListAscTraits<TX>>;

    ofstream("circularlinkedlist.txt", ios::trunc).close();

    // Pruebas de pushback
    cout <<"-- PRUEBAS DE PUSHBACK --\n";
    IntCircularLinkedList list;
    TestContainer(list, {{5, 15}, {1, 11}, {8, 18}, {3, 13}}, "circularlinkedlist.txt");
    TestForwardTraversal(list);

    auto elements = std::vector<pair<TX, Ref>>({{0, 10}, {1, 11}, {2, 12}, {3, 13}, {4, 14}});
    for(auto& e : elements)
        list.push_back(e.first, e.second);
    cout << "CircularLinkedList luego de hacer 5 pushback:\n\t" << list << endl;
    
    // Pruebas de insert
    cout <<"-- PRUEBAS DE INSERT --\n";
    list.insert(4, 12);
    list.insert(11, 20);
    list.insert(7, 6);
    cout << "CircularLinkedList luego de hacer 3 inserts:\n\t" << list << endl;

    // Prueba de apply function
    cout <<"-- PRUEBA DE APPLY FUNCTION --\n";
    list.ApplyFunction(AddOne);
    cout << "CircularLinkedList tras usar Add One a sus elementos:\n\t" << list << endl;
    list.ApplyFunction(AddX<TX>, TX(10));
    cout << "CircularLinkedList tras usar AddX(10) a sus elementos:\n\t" << list << endl;

    // Prueba de clear
    cout <<"-- PRUEBAS DE CLEAR Y REINSERCION --\n";
    list.clear();
    cout << "CircularLinkedList luego de usar clear:\n\t" << list << endl;
    list.push_back(10, 2);
    cout << "CircularLinkedList luego de reinsertar:\n\t" << list << endl;

    // Prueba de lectura
    cout <<"-- PRUEBA DE LECTURA --\n";
    IntCircularLinkedList new_list;
    ifstream in("circularlinkedlist.txt");
    in >> new_list;
    in.close();
    cout << "CircularLinkedList leida desde archivo:\n\t" << new_list << endl;
    TestForwardTraversal(new_list);
}

void DemoDoublyLinkedList()
{
    using IntDoublyLinkedList = DoublyLinkedList<DoublyLinkedListAscTraits<TX>>;

    ofstream("doublylinkedlist.txt", ios::trunc).close();

    cout << "-- PRUEBAS DE PUSH_BACK --\n";
    IntDoublyLinkedList list;
    TestContainer(list, {{5, 15}, {1, 11}, {8, 18}, {3, 13}}, "doublylinkedlist.txt");
    TestForwardTraversal(list);

    cout << "-- PRUEBAS DE INSERT --\n";
    list.insert(4, 12);
    list.insert(11, 20);
    list.insert(7, 6);
    cout << "DoublyLinkedList luego de hacer 3 inserts:\n\t" << list << endl;

    cout << "Backward traversal:\n\t[";
    list.rcall(PrintNode<typename IntDoublyLinkedList::Node>, cout);
    cout << "]" << endl;

    cout << "-- PRUEBAS DE APPLY FUNCTION --\n";
    list.ApplyFunction(AddOne);
    cout << "DoublyLinkedList tras usar AddOne:\n\t" << list << endl;

    cout << "-- PRUEBAS DE CLEAR Y REINSERCION --\n";
    list.clear();
    cout << "DoublyLinkedList luego de clear:\n\t" << list << endl;
    list.push_back(10, 2);
    list.push_back(5, 1);
    cout << "DoublyLinkedList luego de reinsertar:\n\t" << list << endl;

    cout << "-- PRUEBA DE LECTURA --\n";
    IntDoublyLinkedList loadedList;
    ifstream in("doublylinkedlist.txt");
    in >> loadedList;
    in.close();
    cout << "DoublyLinkedList leida desde archivo:\n\t" << loadedList << endl;
    TestForwardTraversal(loadedList);
}

void DemoCircularDoublyLinkedList()
{
    using IntCircularDoublyLinkedList =
        CircularDoublyLinkedList<CircularDoublyLinkedListAscTraits<TX>>;

    ofstream("circulardoublylinkedlist.txt", ios::trunc).close();

    cout << "-- PRUEBAS DE PUSH_BACK --\n";
    IntCircularDoublyLinkedList list;
    TestContainer(list, {{5, 15}, {1, 11}, {8, 18}, {3, 13}},
                  "circulardoublylinkedlist.txt");
    TestForwardTraversal(list);

    cout << "-- PRUEBAS DE INSERT --\n";
    list.insert(4, 12);
    list.insert(11, 20);
    list.insert(7, 6);
    cout << "CircularDoublyLinkedList luego de insertar:\n\t" << list << endl;

    cout << "Backward traversal:\n\t[";
    list.rcall(PrintNode<typename IntCircularDoublyLinkedList::Node>, cout);
    cout << "]" << endl;

    cout << "-- PRUEBAS DE APPLY FUNCTION --\n";
    list.ApplyFunction(AddOne);
    cout << "CircularDoublyLinkedList tras usar AddOne:\n\t" << list << endl;

    cout << "-- PRUEBAS DE CLEAR Y REINSERCION --\n";
    list.clear();
    cout << "CircularDoublyLinkedList luego de clear:\n\t" << list << endl;
    list.push_back(10, 2);
    list.push_back(5, 1);
    cout << "CircularDoublyLinkedList despues de reinsertar:\n\t" << list << endl;

    cout << "-- PRUEBA DE LECTURA --\n";
    IntCircularDoublyLinkedList loadedList;
    ifstream in("circulardoublylinkedlist.txt");
    in >> loadedList;
    in.close();
    cout << "CircularDoublyLinkedList leida desde archivo:\n\t" << loadedList << endl;
    TestForwardTraversal(loadedList);
    cout << "Backward traversal de la lista leida:\n\t[";
    loadedList.rcall(PrintNode<typename IntCircularDoublyLinkedList::Node>, cout);
    cout << "]" << endl;
}

void DemoHeap() {
    using IntMinHeap = MinHeap<TX>;
    using IntMaxHeap = MaxHeap<TX>;

    ofstream("minheap.txt", ios::trunc).close();
    ofstream("maxheap.txt", ios::trunc).close();

    // Pruebas de push_back
    cout << "-- PRUEBAS DE PUSH_BACK (MIN HEAP) --\n";
    IntMinHeap minHeap;
    TestContainer(minHeap, {{5, 15}, {1, 11}, {8, 18}, {3, 13},
                            {9, 19}, {2, 12}, {7, 17}}, "minheap.txt");

    cout << "-- PRUEBAS DE PUSH_BACK (MAX HEAP) --\n";
    IntMaxHeap maxHeap;
    TestContainer(maxHeap, {{5, 15}, {1, 11}, {8, 18}, {3, 13},
                            {9, 19}, {2, 12}, {7, 17}}, "maxheap.txt");

    // Pruebas de recorrido
    cout << "-- PRUEBAS DE RECORRIDO --\n";

    cout << "MinHeap recorrido hacia adelante:\n\t[";
    minHeap.call(PrintNode<typename IntMinHeap::Node>, cout);
    cout << "]" << endl;

    cout << "MaxHeap recorrido hacia adelante:\n\t[";
    maxHeap.call(PrintNode<typename IntMaxHeap::Node>, cout);
    cout << "]" << endl;

    // Pruebas de ApplyFunction
    cout << "-- PRUEBAS DE APPLY FUNCTION --\n";

    minHeap.ApplyFunction(AddOne);
    cout << "MinHeap tras usar AddOne:\n\t" << minHeap << endl;

    maxHeap.ApplyFunction(AddX<TX>, TX(10));
    cout << "MaxHeap tras usar AddX(10):\n\t" << maxHeap << endl;

    cout << "-- PRUEBAS DE POP_BACK --\n";

    cout << "Extrayendo elementos del MinHeap:\n";
    while (!minHeap.empty()) {
        auto node = minHeap.pop();
        cout << "\tExtraido: " << node
             << " | Heap restante: " << minHeap << endl;
    }

    cout << "Extrayendo elementos del MaxHeap:\n";
    while (!maxHeap.empty()) {
        auto node = maxHeap.pop();
        cout << "\tExtraido: " << node
             << " | Heap restante: " << maxHeap << endl;
    }

    cout << "-- PRUEBA DE HEAP VACIO --\n";

    try {
        minHeap.pop();
    } catch (const std::out_of_range& e) {
        cout << "Excepcion esperada: " << e.what() << endl;
    }

    cout << "-- PRUEBAS DE REINSERCION --\n";

    minHeap.push_back(10, 2);
    minHeap.push_back(4, 1);
    minHeap.push_back(15, 3);

    cout << "MinHeap luego de reinsertar:\n\t" << minHeap << endl;

    cout << "-- PRUEBAS DE CLEAR --\n";

    minHeap.clear();
    cout << "MinHeap luego de clear:\n\t" << minHeap << endl;

    cout << "-- PRUEBAS DE LECTURA --\n";

    IntMinHeap loadedMinHeap;
    ifstream inMin("minheap.txt");
    inMin >> loadedMinHeap;
    inMin.close();

    cout << "MinHeap leido desde archivo:\n\t"
         << loadedMinHeap << endl;

    IntMaxHeap loadedMaxHeap;
    ifstream inMax("maxheap.txt");
    inMax >> loadedMaxHeap;
    inMax.close();

    cout << "MaxHeap leido desde archivo:\n\t"
         << loadedMaxHeap << endl;

    cout << "-- EXTRACCION DE HEAPS LEIDOS --\n";

    cout << "MinHeap:\n\t";
    while (!loadedMinHeap.empty())
        cout << loadedMinHeap.pop() << " ";
    cout << endl;

    cout << "MaxHeap:\n\t";
    while (!loadedMaxHeap.empty())
        cout << loadedMaxHeap.pop() << " ";
    cout << endl;
}

void DemoBinaryTree() {
    using IntAscTree  = BinaryTree<BinaryTreeAscTraits<TX>>;
    using IntDescTree = BinaryTree<BinaryTreeDescTraits<TX>>;

    ofstream("binarytree_asc.txt", ios::trunc).close();
    ofstream("binarytree_desc.txt", ios::trunc).close();

    cout << "-- PRUEBAS DE INSERT (ASCENDING TREE) --\n";
    IntAscTree ascTree;

    TestContainer(ascTree,
        {{8, 18}, {3, 13}, {10, 20}, {1, 11},
         {6, 16}, {14, 24}, {4, 14}, {7, 17}, {13, 23}},
        "binarytree_asc.txt");

    cout << "-- PRUEBAS DE INSERT (DESCENDING TREE) --\n";
    IntDescTree descTree;

    TestContainer(descTree,
        {{8, 18}, {3, 13}, {10, 20}, {1, 11},
         {6, 16}, {14, 24}, {4, 14}, {7, 17}, {13, 23}},
        "binarytree_desc.txt");

    cout << "-- PRUEBAS DE RECORRIDO (ASCENDING TREE) --\n";
    TestTraversal(ascTree);

    cout << "-- PRUEBAS DE RECORRIDO (DESCENDING TREE) --\n";
    TestTraversal(descTree);

    cout << "-- PRUEBAS DE ITERADORES --\n";

    cout << "AscTree usando range-based for (preorder):\n\t[";
    for (auto&& node : ascTree)
        cout << node << " ";
    cout << "]" << endl;

    cout << "AscTree usando rbegin/rend (postorder right-first):\n\t[";
    for (auto it = ascTree.rbegin(); it != ascTree.rend(); ++it)
        cout << *it << " ";
    cout << "]" << endl;

    cout << "-- PRUEBAS DE REINSERCION --\n";

    ascTree.insert(5, 15);
    ascTree.insert(2, 12);
    ascTree.insert(12, 22);

    cout << "AscTree luego de 3 inserts:\n\t" << ascTree << endl;

    cout << "Preorder despues de insertar:\n\t[";
    ascTree.call(PrintNode<typename IntAscTree::Node>, cout);
    cout << "]" << endl;

    cout << "-- PRUEBAS DE COPY CONSTRUCTOR --\n";

    IntAscTree copiedTree(ascTree);

    cout << "Arbol original:\n" << ascTree << endl;
    cout << "Arbol copiado:\n" << copiedTree << endl;

    copiedTree.insert(20, 30);

    cout << "Arbol copiado luego de insertar 20:\n"
         << copiedTree << endl;
    cout << "Arbol original (sin cambios):\n"
         << ascTree << endl;

    cout << "-- PRUEBAS DE MOVE CONSTRUCTOR --\n";

    IntAscTree movedTree(std::move(copiedTree));

    cout << "Arbol movido:\n" << movedTree << endl;
    cout << "Tamanio del arbol origen: " << copiedTree.size() << endl;

    cout << "-- PRUEBAS DE CLEAR --\n";

    ascTree.clear();

    cout << "AscTree luego de clear:\n" << ascTree << endl;
    cout << "Tamanio: " << ascTree.size() << endl;

    ascTree.insert(10, 2);
    ascTree.insert(5, 1);
    ascTree.insert(15, 3);

    cout << "AscTree luego de reinsertar:\n"
         << ascTree << endl;

    cout << "-- PRUEBAS DE LECTURA --\n";

    IntAscTree loadedTree;
    ifstream in("binarytree_asc.txt");

    in >> loadedTree;
    in.close();

    cout << "BinaryTree leido desde archivo:\n"
         << loadedTree << endl;

    cout << "-- RECORRIDOS DEL ARBOL LEIDO --\n";
    TestTraversal(loadedTree);

    cout << "-- PRUEBAS DE ARBOL VACIO --\n";

    IntAscTree emptyTree;

    cout << "Arbol vacio, size = " << emptyTree.size() << endl;
    cout << "begin() == end(): "
         << (emptyTree.begin() == emptyTree.end()) << endl;
}