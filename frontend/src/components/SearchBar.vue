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

const emits = defineEmits(['search', 'update:modelValue'])
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
        class="flex-1 max-w-sm bg-bg-secondary border border-border text-text-primary px-4 py-2.5 rounded-lg text-sm outline-none transition focus:border-accent"
      />
</template>