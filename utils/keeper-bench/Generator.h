#pragma once
#include <Common/ZooKeeper/KeeperException.h>
#include <Common/ZooKeeper/ZooKeeper.h>
#include <Common/ZooKeeper/ZooKeeperImpl.h>
#include <Common/ZooKeeper/ZooKeeperCommon.h>
#include <functional>
#include <optional>
#include <pcg-random/pcg_random.hpp>
#include <Common/randomSeed.h>


std::string generateRandomPath(const std::string & prefix, size_t length = 5);

std::string generateRandomData(size_t size);

class IGenerator
{
public:
    IGenerator()
    {
        Coordination::ACL acl;
        acl.permissions = Coordination::ACL::All;
        acl.scheme = "world";
        acl.id = "anyone";
        default_acls.emplace_back(std::move(acl));
    }
    virtual void startup(Coordination::ZooKeeper & /*zookeeper*/) {}
    virtual Coordination::ZooKeeperRequestPtr generate() = 0;

    virtual ~IGenerator() = default;

    Coordination::ACLs default_acls;

};

class CreateRequestGenerator final : public IGenerator
{
public:
    explicit CreateRequestGenerator(
        std::string path_prefix_ = "/create_generator",
        std::optional<uint64_t> path_length_ = std::nullopt,
        std::optional<uint64_t> data_size_ = std::nullopt)
        : path_prefix(path_prefix_)
        , path_length(path_length_)
        , data_size(data_size_)
    {}

    void startup(Coordination::ZooKeeper & zookeeper) override;
    Coordination::ZooKeeperRequestPtr generate() override;

private:
    std::string path_prefix;
    std::optional<uint64_t> path_length;
    std::optional<uint64_t> data_size;
    std::unordered_set<std::string> paths_created;
};


class GetRequestGenerator final : public IGenerator
{
public:
    explicit GetRequestGenerator(
        std::string path_prefix_ = "/get_generator",
        std::optional<uint64_t> num_nodes_ = std::nullopt,
        std::optional<uint64_t> nodes_data_size_ = std::nullopt)
        : path_prefix(path_prefix_)
        , num_nodes(num_nodes_)
        , nodes_data_size(nodes_data_size_)
        , rng(randomSeed())
        , distribution(0, num_nodes ? *num_nodes - 1 : 0)
    {}

    void startup(Coordination::ZooKeeper & zookeeper) override;
    Coordination::ZooKeeperRequestPtr generate() override;

private:
    std::string path_prefix;
    std::optional<uint64_t> num_nodes;
    std::optional<uint64_t> nodes_data_size;
    std::vector<std::string> paths_to_get;

    pcg64 rng;
    std::uniform_int_distribution<size_t> distribution;
};

class ListRequestGenerator final : public IGenerator
{
public:
    explicit ListRequestGenerator(
        std::string path_prefix_ = "/list_generator",
        std::optional<uint64_t> num_nodes_ = std::nullopt,
        std::optional<uint64_t> paths_length_ = std::nullopt)
        : path_prefix(path_prefix_)
        , num_nodes(num_nodes_)
        , paths_length(paths_length_)
    {}

    void startup(Coordination::ZooKeeper & zookeeper) override;
    Coordination::ZooKeeperRequestPtr generate() override;

private:
    std::string path_prefix;
    std::optional<uint64_t> num_nodes;
    std::optional<uint64_t> paths_length;
};

class SetRequestGenerator final : public IGenerator
{
public:
    explicit SetRequestGenerator(
        std::string path_prefix_ = "/set_generator",
        uint64_t data_size_ = 5)
        : path_prefix(path_prefix_)
        , data_size(data_size_)
    {}

    void startup(Coordination::ZooKeeper & zookeeper) override;
    Coordination::ZooKeeperRequestPtr generate() override;

private:
    std::string path_prefix;
    uint64_t data_size;
};

class MixedRequestGenerator final : public IGenerator
{
public:
    explicit MixedRequestGenerator(std::vector<std::unique_ptr<IGenerator>> generators_)
        : generators(std::move(generators_))
    {}

    void startup(Coordination::ZooKeeper & zookeeper) override;
    Coordination::ZooKeeperRequestPtr generate() override;

private:
    std::vector<std::unique_ptr<IGenerator>> generators;
};


std::unique_ptr<IGenerator> getGenerator(const std::string & name);
