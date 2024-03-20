#include "StoreSell.h"

#include <Points.h>

#include "ArkShop.h"
#include "DBHelper.h"
#include "ShopLog.h"

namespace ArkShop::StoreSell
{
	bool SellItem(AShooterPlayerController* player_controller, const nlohmann::basic_json<>& item_entry,
		const FString& eos_id,
		int amount)
	{
		bool success = false;

		const int price = item_entry.value("Price", 1) * amount;
		if (price <= 0)
		{
			return false;
		}

		const FString blueprint = FString(item_entry.value("Blueprint", "").c_str());
		const int needed_amount = item_entry.value("Amount", 1) * amount;
		if (needed_amount <= 0)
		{
			return false;
		}

		UPrimalInventoryComponent* inventory = player_controller->GetPlayerCharacter()->MyInventoryComponentField();
		if (inventory == nullptr)
		{
			return false;
		}

		int item_count = 0;

		// Count items

		TArray<UPrimalItem*> items_for_removal;

		TArray<UPrimalItem*> items = inventory->InventoryItemsField();
		for (UPrimalItem* item : items)
		{
			if (item->ClassPrivateField() && item->bAllowRemovalFromInventory()() && !item->bIsEngram()())
			{
				const FString item_bp = AsaApi::GetApiUtils().GetItemBlueprint(item);

				if (item_bp == blueprint)
				{
					items_for_removal.Add(item);

					item_count += item->GetItemQuantity();
					if (item_count >= needed_amount)
					{
						break;
					}
				}
			}
		}

		if (item_count >= needed_amount)
		{
			item_count = 0;

			// Remove items
			for (UPrimalItem* item : items_for_removal)
			{
				item_count += item->GetItemQuantity();

				if (item_count > needed_amount)
				{
					item->SetQuantity(item_count - needed_amount, true);
					inventory->NotifyClientsItemStatus(item, false, false, true, false, false, nullptr, nullptr, false,
						false, true);
				}
				else
				{
					inventory->RemoveItem(&item->ItemIDField(), false, false, true, true);
				}
			}

			if (!Points::AddPoints(price, eos_id))
			{
				ShopLog::GetLog()->error("Unexpected error when selling {} for {}", blueprint.ToString(), eos_id.ToString());
				return false;
			}

			AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("SoldItems"));

			success = true;
		}
		else
		{
			AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"), *GetText("NotEnoughItems"),
				item_count,
				needed_amount);
		}

		return success;
	}

	bool Sell(AShooterPlayerController* player_controller, const FString& item_id, int amount)
	{
		if (AsaApi::IApiUtils::IsPlayerDead(player_controller))
		{
			return false;
		}

		if (amount <= 0)
		{
			amount = 1;
		}

		bool success = false;

		const FString& eos_id = AsaApi::IApiUtils::GetEOSIDFromController(player_controller);

		if (DBHelper::IsPlayerExists(eos_id))
		{
			auto items_list = config.value("SellItems", nlohmann::json::object());

			auto item_entry_iter = items_list.find(item_id.ToString());
			if (item_entry_iter == items_list.end())
			{
				AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
					*GetText("WrongId"));
				return false;
			}

			auto item_entry = item_entry_iter.value();

			const std::string type = item_entry["Type"];

			if (type == "item")
			{
				success = SellItem(player_controller, item_entry, eos_id, amount);
			}

			if (success)
			{
				const std::wstring log = fmt::format(TEXT("[{}] {}({}) Sold item: '{}' Amount: {}"),
					*ArkShop::SetMapName(),
					*AsaApi::IApiUtils::GetSteamName(player_controller), eos_id.ToString(),
					*item_id,
					amount);

				ShopLog::GetLog()->info(AsaApi::Tools::Utf8Encode(log));
				ArkShop::PostToDiscord(log);
			}
		}

		return success;
	}

	// Chat callbacks

	void ChatSell(AShooterPlayerController* player_controller, FString* message, int, int)
	{
		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		if (parsed.IsValidIndex(1))
		{
			int amount = 0;

			if (parsed.IsValidIndex(2))
			{
				try
				{
					amount = std::stoi(*parsed[2]);
				}
				catch (const std::exception& exception)
				{
					Log::GetLog()->warn("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
					return;
				}
			}

			Sell(player_controller, parsed[1], amount);
		}
		else
		{
			AsaApi::GetApiUtils().SendChatMessage(player_controller, GetText("Sender"),
				*GetText("SellUsage"));
		}
	}

	void ShowItems(AShooterPlayerController* player_controller, FString* message, int, int)
	{
		if (AsaApi::Tools::IsPluginLoaded("ArkShopUI"))
			return;

		TArray<FString> parsed;
		message->ParseIntoArray(parsed, L" ", true);

		int page = 0;

		if (parsed.IsValidIndex(1))
		{
			try
			{
				page = std::stoi(*parsed[1]) - 1;
			}
			catch (const std::exception&)
			{
				return;
			}
		}

		if (page < 0)
		{
			return;
		}

		auto items_list = config.value("SellItems", nlohmann::json::object());

		const int items_per_page = config["General"].value("ItemsPerPage", 20);
		const float display_time = config["General"].value("ShopDisplayTime", 15.0f);
		const float text_size = config["General"].value("ShopTextSize", 1.3f);

		const unsigned start_index = page * items_per_page;
		if (start_index >= items_list.size())
		{
			return;
		}

		auto start = items_list.begin();
		advance(start, start_index);

		FString store_str = "";

		for (auto iter = start; iter != items_list.end(); ++iter)
		{
			const size_t i = distance(items_list.begin(), iter);
			if (i == start_index + items_per_page)
			{
				break;
			}

			auto item = iter.value();

			const int price = item["Price"];
			const std::string type = item["Type"];
			const std::string description = item.value("Description", "No description");

			store_str += FString::Format(*GetText("StoreListItem"), i + 1, description,
				AsaApi::Tools::Utf8Decode(iter.key()),
				price);
		}

		AsaApi::GetApiUtils().SendNotification(player_controller, FColorList::White, text_size, display_time, nullptr,
			*store_str);
	}

	// Console callbacks

	void ListInvItemsCmd(APlayerController* controller, FString* /*cmd*/, bool /*unused*/)
	{
		const auto shooter_controller = static_cast<AShooterPlayerController*>(controller);
		AShooterCharacter* character = shooter_controller->GetPlayerCharacter();
		if (!character)
		{
			return;
		}

		UPrimalInventoryComponent* inventory = character->MyInventoryComponentField();
		if (!inventory)
		{
			return;
		}

		TArray<UPrimalItem*> items = inventory->InventoryItemsField();
		for (UPrimalItem* item : items)
		{
			if (item->ClassPrivateField() != nullptr)
			{
				const FString item_bp = AsaApi::GetApiUtils().GetItemBlueprint(item);
				Log::GetLog()->info(item_bp.ToString());
			}
		}
	}

	void Init()
	{
		AsaApi::GetCommands().AddChatCommand(GetText("SellCmd"), &ChatSell);
		AsaApi::GetCommands().AddChatCommand(GetText("ShopSellCmd"), &ShowItems);

		AsaApi::GetCommands().AddConsoleCommand(L"ListInvItems", &ListInvItemsCmd);
	}

	void Unload()
	{
		AsaApi::GetCommands().RemoveChatCommand(GetText("SellCmd"));
		AsaApi::GetCommands().RemoveChatCommand(GetText("ShopSellCmd"));

		AsaApi::GetCommands().RemoveConsoleCommand(L"ListInvItems");
	}
} // namespace StoreSell // namespace ArkShop