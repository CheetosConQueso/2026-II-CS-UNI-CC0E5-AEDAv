
#include <iostream>
#include <fstream> // ofstream para escribir en archivo
#include <string>
#include <thread>
#include <vector>
#include "foreach.h"
#include "containers/vector.h"
#include "containers/LinkedList.h"
#include "containers/CircularLinkedList.h"
#include "containers/DoublyLinkedList.h"
#include "containers/CircularDoublyLinkedList.h"
#include "Demos.h"
using namespace std;

void AddOne(GeneralNode<TX> &node) {
    node.value() += 1;
}

template <typename T>
void AddX(GeneralNode<T> &node, T x) {
    node.value() += x;
}

void Square(GeneralNode<TX> &node) {
    node.value() *= node.value();
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
    using Node = typename Container::Node;

    cout << "Forward traversal:\n\t[";
    container.call(PrintNode<Node>, cout);
    cout << "]" << endl;

    cout << "Backward traversal:\n\t[";
    container.rcall(PrintNode<Node>, cout);
    cout << "]" << endl;

    // Prueba de call()/FirstThat() con valor de retorno: busca el primer
    // nodo cuyo valor sea mayor que el del primer elemento del container.
    if (!container.empty()) {
        auto threshold = (*container.begin()).getValue();
        Node &found = container.FirstThat(IsGreaterThan<Node, decltype(threshold)>, threshold);
        cout << "FirstThat (primer valor > " << threshold << "):\n\t" << found << endl;
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
