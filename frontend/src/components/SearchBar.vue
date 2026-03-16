<script setup>
import { ref } from 'vue'

const props = defineProps({
  text: {
    type: String,
    default: ''
  },
  modelValue: {
    type: String,
    default: ''
  }
})

const emits = defineEmits(['search', 'update:modelValue', 'focused'])
const query = ref(props.modelValue)

function search() {
  const sym = query.value.trim().toUpperCase()
  if (sym) {
    emits('search', sym)
  }
}

</script>
<template>
    <input
        v-model="query"
        :placeholder="text"
        @input="emits('update:modelValue', query)"
        @keydown.enter="search"
        @focus="emits('focused', true)"
        @blur="emits('focused', false)"
        class="search-input flex-1 max-w-sm bg-bg-secondary border border-border text-text-primary px-4 py-2.5 rounded-lg text-sm outline-none transition"
      />
</template>

<style scoped>
.search-input {
  transition: border-color 0.2s ease, box-shadow 0.2s ease;
}

.search-input:focus {
  border-color: rgba(108, 92, 231, 0.8);
  box-shadow: 0 0 0 3px rgba(108, 92, 231, 0.2), 0 0 16px rgba(108, 92, 231, 0.15);
}
</style>