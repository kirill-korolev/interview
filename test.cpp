#include <cstdlib>
#include <string>
#include <ctime>
#include <iostream>
#include <sys/time.h>
#include <list>
#include <fstream>
#include <cstddef>
#include <vector>

using write_sequence = std::list<std::string>;
using read_sequence = std::list<std::pair<uint64_t, std::string>>;

uint64_t get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

write_sequence get_write_sequence()
{
    std::ifstream infile("insert.txt");
    write_sequence result;

    std::string item;

    while (infile >> item)
    {
        result.emplace_back(item);
    }

    return result;
}

read_sequence get_read_sequence()
{
    std::ifstream infile("read.txt");
    read_sequence result;

    uint64_t index;
    std::string str;

    while(infile >> index >> str)
    {
        result.emplace_back(std::make_pair(index, str));
    }

    return result;
}

class storage
{
private:
    // node struct represents a node in a prefix tree
    struct node
    {
        typedef std::vector<node*> children_container;
        typedef typename children_container::iterator children_iterator;

        // comparator for binary search on node's children
        struct comp
        {
            bool operator()(const node *lhs, char rhs) const
            {
                return lhs->character < rhs;
            }
            bool operator()(char lhs, const node *rhs) const
            {
                return lhs < rhs->character;
            }
        };

        explicit node(char c=0): parent{nullptr}, num_words{0}, is_leaf{false}, character(c){}

        ~node()
        {
            for(auto child: children)
            {
                child->~node();
            }
        }

        // performs a binary search for a given character among node's children
        children_iterator find_child(char c)
        {
            return std::lower_bound(children.begin(), children.end(), c, comp());
        }

        // inserts a new node with a given character
        // to a specific place in a children sequence
        children_iterator create_child(char c)
        {
            auto new_node = new node(c);
            auto child = find_child(c);
            new_node->parent = this;
            return children.insert(child, new_node);
        }

        // converts a node to a leaf, stores a given string
        // and updates number of words upwards to the root
        void convert_to_leaf(const std::string &str)
        {
            this->is_leaf = true;
            this->num_words = 1;
            this->str = str;

            node *p = parent;

            while(p)
            {
                ++p->num_words;
                p = p->parent;
            }
        }

        children_container children; // sorted sequence of node's children
        node *parent;
        uint64_t num_words; // number of words in a subtrees
        bool is_leaf;
        char character;
        std::string str; // stores a string representation of prefix if it's a leaf, empty otherwise
    };
public:
    storage(): root_{new node}{}

    ~storage()
    {
        delete root_;
    }

    void insert(const std::string &str)
    {
        node *current = root_;

        for(char c: str)
        {
            auto child = current->find_child(c);

            if(child == current->children.end() || (*child)->character != c)
            {
                child = current->create_child(c); // if the node is not found, create it
            }

            current = *child;
        }

        current->convert_to_leaf(str); // propagate node info to the root
    }

    const std::string &get(uint64_t index)
    {
        node *current = root_;

        while(!current->is_leaf)
        {
            auto child = current->children.begin();

            while(child != current->children.end())
            {
                if((*child)->num_words >= index + 1) // select subtree with larger amount of words than index
                {
                    break;
                }
                index -= (*child)->num_words; // skip words if we're iterating over node's children subtrees
                ++child;
            }

            if(child == current->children.end())
            {
                throw std::out_of_range("storage::get");
            }

            current = *child;
        }
        return current->str;
    }

    void erase(const std::string &str)
    {
        node *current = root_;

        for(char c: str)
        {
            auto child = current->find_child(c);

            if(child == current->children.end() || (*child)->character != c)
            {
                return; // do nothing if the word cannot be found
            }

            current = *child;
        }

        if(!current->is_leaf)
        {
            return;
        }

        // traverse to the root while there is only one child
        // delete it and update number of words in a parent
        while(current)
        {
            auto temp = current;
            current = current->parent;
            delete temp;
            --current->num_words;

            if(current->children.size() > 1)
            {
                break;
            }
        }
    }
private:
    node *root_;
};

void test(const write_sequence &write, const read_sequence &read)
{
    storage st;

    uint64_t timestamp_us;
    uint64_t total_time = 0;

    write_sequence::const_iterator iitr = write.begin();
    read_sequence::const_iterator ritr = read.begin();
    while (iitr != write.end() && ritr != read.end())
    {
        timestamp_us = get_time();
        st.insert(*iitr);
        total_time += get_time() - timestamp_us;

        timestamp_us = get_time();
        const std::string &str = st.get(ritr->first);
        total_time += get_time() - timestamp_us;

        if (ritr->second != str)
        {
            std::cout << "test failed" << std::endl;
            return;
        }

        iitr++;
        ritr++;
    }

    std::cout << "total time: " << total_time / 1e6 << std::endl;
}


int main()
{
    write_sequence insert =  get_write_sequence();
    read_sequence read = get_read_sequence();

    test(insert, read);
}
