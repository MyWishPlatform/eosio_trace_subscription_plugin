#include <eosio/trace_subscription_plugin/trace_subscription_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/tcp_plugin/tcp_plugin.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <boost/signals2/connection.hpp>
#include <fc/io/json.hpp>

namespace eosio {
	static appbase::abstract_plugin& _trace_subscription_plugin = app().register_plugin<trace_subscription_plugin>();

	class trace_subscription_plugin_impl {
	private:
		struct client_t {
			boost::asio::ip::tcp::socket* const socket;
			std::string addr;
		};

		chain_plugin& chain_plugin_ref;
		tcp_plugin& tcp_plugin_ref;
		std::function<fc::optional<abi_serializer>(const account_name&)> resolver;
		std::vector<client_t*> clients;
		boost::asio::deadline_timer timer;
		std::mutex mutex;
		fc::optional<boost::signals2::scoped_connection> applied_transaction_connection;

		std::string trace_to_json(const chain::transaction_trace& trace) const {
			fc::variant output;
			abi_serializer::to_variant(trace, output, this->resolver, this->chain_plugin_ref.get_abi_serializer_max_time());
			return fc::json::to_string(fc::mutable_variant_object(output.get_object()));
		}

		void on_trace(const chain::transaction_trace& trace) {
			this->mutex.lock();
			try {
				std::for_each(this->clients.begin(), this->clients.end(), [this, trace](client_t* client) {
					this->tcp_plugin_ref.send(client->socket, this->trace_to_json(trace));
				});
			} catch (...) {}
			this->mutex.unlock();
		}

	public:
		trace_subscription_plugin_impl() :
			chain_plugin_ref(app().get_plugin<chain_plugin>()),
			tcp_plugin_ref(app().get_plugin<tcp_plugin>()),
			resolver([this](const account_name& name) -> fc::optional<abi_serializer> {
				const chain::account_object* account = this->chain_plugin_ref.chain().db().find<chain::account_object, chain::by_name>(name);
				auto time = this->chain_plugin_ref.get_abi_serializer_max_time();
				if (account != nullptr) {
					abi_def abi;
					if (abi_serializer::to_abi(account->abi, abi)) {
						return abi_serializer(abi, time);
					}
				}
				return fc::optional<abi_serializer>();
			}),
			timer(app().get_io_service(), boost::posix_time::seconds(1))
		{
			this->tcp_plugin_ref.add_callback_msg([this](boost::asio::ip::tcp::socket* const socket, std::stringstream data) {
				char msg;
				data >> msg;
				switch (msg) {
					case 't': {
						for (client_t* client : this->clients) {
							if (client->socket == socket) return;
						}
						client_t* client = new client_t{
							.socket = socket,
							.addr = socket->remote_endpoint().address().to_string()
						};
						this->mutex.lock();
						this->clients.push_back(client);
						this->mutex.unlock();
						ilog("client '" + client->addr + "' subscribed to transactions");
						break;
					}
					default: {
						break;
					}
				}
			});
			this->tcp_plugin_ref.add_callback_disconnect([this](boost::asio::ip::tcp::socket* const socket) {
				this->mutex.lock();
				this->clients.erase(std::remove_if(this->clients.begin(), this->clients.end(), [socket](client_t* client) {
					if (client->socket == socket) {
						ilog("client '" + client->addr + "' unsubscribed from transactions");
						delete client;
						return true;
					}
					return false;
				}), this->clients.end());
				this->mutex.unlock();
			});
			this->applied_transaction_connection.emplace(
				this->chain_plugin_ref.chain().applied_transaction.connect([this](const auto& trace) {
					this->on_trace(*trace);
				})
			);
		}

		~trace_subscription_plugin_impl() {
			this->applied_transaction_connection.reset();
		}
	};

	trace_subscription_plugin::trace_subscription_plugin() {}

	trace_subscription_plugin::~trace_subscription_plugin() {}

	void trace_subscription_plugin::set_program_options(options_description&, options_description& cfg) {}

	void trace_subscription_plugin::plugin_initialize(const variables_map& options) {
		ilog("starting trace_subscription_plugin");
		this->my = new trace_subscription_plugin_impl();
	}

	void trace_subscription_plugin::plugin_startup() {}

	void trace_subscription_plugin::plugin_shutdown() {
		delete this->my;
	}
}
