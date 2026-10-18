#ifndef EQEMU_HOUSING_TRANSFER_PENDING_H
#define EQEMU_HOUSING_TRANSFER_PENDING_H
#include "../common/housing/ownership_transfer_service.h"
#include "../common/housing/transfer_consent_wire.h"
#include <chrono>
class EQStreamInterface;
namespace Housing::TransferConsent {
struct Pending {
 OwnershipTransfer::Receipt inspected;
 TransferConsentWire::Body offer{};
 EQStreamInterface* sender_connection=nullptr;
 EQStreamInterface* recipient_connection=nullptr;
 std::chrono::steady_clock::time_point expires;
 bool Expired()const{return std::chrono::steady_clock::now()>=expires;}
};
}
#endif
